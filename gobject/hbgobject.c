/*
 * hbgi source code
 * Core code
 *
 * Copyright 2014-2016 Phil Krylov <phil.krylov a t gmail.com>
 *
 * Most of the logic in this file is based on pygobject.c from pygobject
 * library:
 *
 * pygtk- Python bindings for the GTK toolkit.
 * Copyright (C) 1998-2003  James Henstridge
 *
 *   pygobject.c: wrapper for the GObject type.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */

#include <hbapi.h>
#include <hbapicls.h>
#include <hbapierr.h>
#include <hbapiitm.h>
#include <hbinit.h>
#include <hboo.ch>
#include <hbstack.h>
#include <hbvm.h>

#include "hbgihb.h"

#include <glib-object.h>

#include "hbginterface.h"
#include "hbgobject.h"
#include "hbgpointer.h"
#include "hbgtype.h"

#include "hbgi.h"

//#define HBGOBJECT_IVAR_DOC 0
//_GTYPE 1
#define HBGOBJECT_IVAR_GOBJECT 2
#define HBGOBJECT_IVAR_COUNT 2

static HB_USHORT
hbgobject_new_with_interfaces(GType gtype);

#if 0
static void pygobject_dealloc(PyGObject *self);
static int  pygobject_traverse(PyGObject *self, visitproc visit, void *arg);
#endif
HB_FUNC_STATIC(hbgobject_clear);
static inline HbGObjectData *
hbgobject_get_inst_data(PHB_ITEM self);
static PHB_ITEM hbg_type_get_bases(GType gtype);
#if 0
static PyObject * pygobject_weak_ref_new(GObject *obj, PyObject *callback, PyObject *user_data);
static PyObject * pygobject_weak_ref_new(GObject *obj, PyObject *callback, PyObject *user_data);
static void pygobject_inherit_slots(PyTypeObject *type, PyObject *bases,
				    gboolean check_for_present);
static void pygobject_find_slot_for(PyTypeObject *type, PyObject *bases, int slot_offset,
				    gboolean check_for_present);
#endif

HB_USHORT HbGObject_Type = 0;

GType HB_TYPE_ITEM = 0;
GQuark hbgobject_class_key;
#if 0
GQuark pygobject_class_init_key;
#endif
GQuark hbgobject_wrapper_key;
#if 0
GQuark pygobject_has_updated_constructor_key;
#endif
GQuark hbgobject_instance_data_key;
GQuark hbgobject_ref_sunk_key;
GQuark hbgobject_private_flags_key;

/**
 * hbg_destroy_notify:
 * @user_data: a PHB_ITEM.
 *
 * A function that can be used as a GDestroyNotify callback that will
 * call hb_itemRelease on the data.
 */
void
hbg_destroy_notify(gpointer user_data)
{
    //PyGILState_STATE state;

    //state = pyglib_gil_state_ensure();
    hb_itemRelease(user_data);
    //pyglib_gil_state_release(state);
}

/* -------------- class <-> wrapper manipulation --------------- */

void
hbgobject_data_free(HbGObjectData *data)
{
    //PyGILState_STATE state = pyglib_gil_state_ensure();
    GSList *closures, *tmp;
    tmp = closures = data->closures;
#ifndef NDEBUG
    data->closures = NULL;
    data->type = 0;
#endif
    //pyg_begin_allow_threads;
    while (tmp) {
 	GClosure *closure = tmp->data;

          /* we get next item first, because the current link gets
           * invalidated by pygobject_unwatch_closure */
 	tmp = tmp->next;
 	g_closure_invalidate(closure);
    }
    //pyg_end_allow_threads;

    if (data->closures != NULL)
 	g_warning("invalidated all closures, but data->closures != NULL !");

    g_free(data);
    //pyglib_gil_state_release(state);
}

static inline HbGObjectData *
hbgobject_data_new(void)
{
    HbGObjectData *data;
    data = g_new0(HbGObjectData, 1);
    return data;
}

static inline HbGObjectData *
hbgobject_get_inst_data(PHB_ITEM self)
{
    HbGObjectData *inst_data;
    GObject *obj = hbgobject_get(self);

    if (G_UNLIKELY(!obj))
        return NULL;
    inst_data = g_object_get_qdata(obj, hbgobject_instance_data_key);
    if (inst_data == NULL)
    {
        inst_data = hbgobject_data_new();

        inst_data->type = hb_objGetClass(self);

        g_object_set_qdata_full(obj, hbgobject_instance_data_key,
                                inst_data, (GDestroyNotify)hbgobject_data_free);
    }
    return inst_data;
}


#if 0
GHashTable *custom_type_registration = NULL;

PyTypeObject *PyGObject_MetaType = NULL;
#endif

/**
 * hbgobject_sink:
 * @obj: a GObject
 *
 * As Harbour handles reference counting for us, the "floating
 * reference" code in GTK is not all that useful.  In fact, it can
 * cause leaks.  This function should be called to remove the floating
 * references on objects on construction.
 **/
void
hbgobject_sink(GObject *obj)
{
    /* We use a gobject data key to avoid running the sink funcs more than once. */
    if (g_object_get_qdata (obj, hbgobject_ref_sunk_key))
        return;

    if (G_IS_INITIALLY_UNOWNED (obj))
        g_object_ref_sink(obj);

    g_object_set_qdata (obj, hbgobject_ref_sunk_key, GINT_TO_POINTER (1));
}


#if 0
typedef struct {
    PyObject_HEAD
    GParamSpec **props;
    guint n_props;
    guint index;
} PyGPropsIter;

PYGLIB_DEFINE_TYPE("gobject.GPropsIter", PyGPropsIter_Type, PyGPropsIter);

static void
pyg_props_iter_dealloc(PyGPropsIter *self)
{
    g_free(self->props);
    PyObject_Del((PyObject*) self);
}

static PyObject*
pygobject_props_iter_next(PyGPropsIter *iter)
{
    if (iter->index < iter->n_props)
        return pyg_param_spec_new(iter->props[iter->index++]);
    else {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }
}

typedef struct {
    PyObject_HEAD
    /* a reference to the object containing the properties */
    PyGObject *pygobject;
    GType      gtype;
} PyGProps;

static void
PyGProps_dealloc(PyGProps* self)
{
    PyGObject *tmp;

    PyObject_GC_UnTrack((PyObject*)self);

    tmp = self->pygobject;
    self->pygobject = NULL;
    Py_XDECREF(tmp);

    PyObject_GC_Del((PyObject*)self);
}

static PyObject*
build_parameter_list(GObjectClass *class)
{
    GParamSpec **props;
    guint n_props = 0, i;
    PyObject *prop_str;
    PyObject *props_list;

    props = g_object_class_list_properties(class, &n_props);
    props_list = PyList_New(n_props);
    for (i = 0; i < n_props; i++) {
	char *name;
	name = g_strdup(g_param_spec_get_name(props[i]));
	/* hyphens cannot belong in identifiers */
	g_strdelimit(name, "-", '_');
	prop_str = PYGLIB_PyUnicode_FromString(name);
	
	PyList_SetItem(props_list, i, prop_str);
	g_free(name);
    }

    if (props)
        g_free(props);
    
    return props_list;
}

static PyObject*
PyGProps_getattro(PyGProps *self, PyObject *attr)
{
    char *attr_name;
    GObjectClass *class;
    GParamSpec *pspec;
    GValue value = { 0, };
    PyObject *ret;

    attr_name = PYGLIB_PyUnicode_AsString(attr);
    if (!attr_name) {
        PyErr_Clear();
        return PyObject_GenericGetAttr((PyObject *)self, attr);
    }

    class = g_type_class_ref(self->gtype);
    
    if (!strcmp(attr_name, "__members__")) {
	return build_parameter_list(class);
    }

    if (self->pygobject != NULL) {
        ret = pygi_get_property_value (self->pygobject, attr_name);
        if (ret != NULL)
            return ret;
    }

    pspec = g_object_class_find_property(class, attr_name);
    g_type_class_unref(class);

    if (!pspec) {
	return PyObject_GenericGetAttr((PyObject *)self, attr);
    }

    if (!(pspec->flags & G_PARAM_READABLE)) {
	PyErr_Format(PyExc_TypeError,
		     "property '%s' is not readable", attr_name);
	return NULL;
    }

    /* If we're doing it without an instance, return a GParamSpec */
    if (!self->pygobject) {
        return pyg_param_spec_new(pspec);
    }
    
    g_value_init(&value, G_PARAM_SPEC_VALUE_TYPE(pspec));
    pyg_begin_allow_threads;
    g_object_get_property(self->pygobject->obj, attr_name, &value);
    pyg_end_allow_threads;
    ret = pyg_param_gvalue_as_pyobject(&value, TRUE, pspec);
    g_value_unset(&value);
    
    return ret;
}

static gboolean
set_property_from_pspec(GObject *obj,
			char *attr_name,
			GParamSpec *pspec,
			PyObject *pvalue)
{
    GValue value = { 0, };

    if (pspec->flags & G_PARAM_CONSTRUCT_ONLY) {
	PyErr_Format(PyExc_TypeError,
		     "property '%s' can only be set in constructor",
		     attr_name);
	return FALSE;
    }	

    if (!(pspec->flags & G_PARAM_WRITABLE)) {
	PyErr_Format(PyExc_TypeError,
		     "property '%s' is not writable", attr_name);
	return FALSE;
    }	

    g_value_init(&value, G_PARAM_SPEC_VALUE_TYPE(pspec));
    if (pyg_param_gvalue_from_pyobject(&value, pvalue, pspec) < 0) {
	PyErr_SetString(PyExc_TypeError,
			"could not convert argument to correct param type");
	return FALSE;
    }

    pyg_begin_allow_threads;
    g_object_set_property(obj, attr_name, &value);
    pyg_end_allow_threads;

    g_value_unset(&value);
    
    return TRUE;
}

PYGLIB_DEFINE_TYPE("gobject.GProps", PyGProps_Type, PyGProps);

static int
PyGProps_setattro(PyGProps *self, PyObject *attr, PyObject *pvalue)
{
    GParamSpec *pspec;
    char *attr_name;
    GObject *obj;
    int ret = -1;
    
    if (pvalue == NULL) {
	PyErr_SetString(PyExc_TypeError, "properties cannot be "
			"deleted");
	return -1;
    }

    attr_name = PYGLIB_PyUnicode_AsString(attr);
    if (!attr_name) {
        PyErr_Clear();
        return PyObject_GenericSetAttr((PyObject *)self, attr, pvalue);
    }

    if (!self->pygobject) {
        PyErr_SetString(PyExc_TypeError,
			"cannot set GOject properties without an instance");
        return -1;
    }

    ret = pygi_set_property_value (self->pygobject, attr_name, pvalue);
    if (ret == 0)
        return 0;
    else if (ret == -1)
        if (PyErr_Occurred())
            return -1;

    obj = self->pygobject->obj;
    pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(obj), attr_name);
    if (!pspec) {
	return PyObject_GenericSetAttr((PyObject *)self, attr, pvalue);
    }

    if (!set_property_from_pspec(obj, attr_name, pspec, pvalue))
	return -1;
				  
    return 0;
}

static int
pygobject_props_traverse(PyGProps *self, visitproc visit, void *arg)
{
    if (self->pygobject && visit((PyObject *) self->pygobject, arg) < 0)
        return -1;
    return 0;
}

static PyObject*
pygobject_props_get_iter(PyGProps *self)
{
    PyGPropsIter *iter;
    GObjectClass *class;

    iter = PyObject_NEW(PyGPropsIter, &PyGPropsIter_Type);
    class = g_type_class_ref(self->gtype);
    iter->props = g_object_class_list_properties(class, &iter->n_props);
    iter->index = 0;
    g_type_class_unref(class);
    return (PyObject *) iter;
}

static Py_ssize_t
PyGProps_length(PyGProps *self)
{
    GObjectClass *class;
    GParamSpec **props;
    guint n_props;
    
    class = g_type_class_ref(self->gtype);
    props = g_object_class_list_properties(class, &n_props);
    g_type_class_unref(class);
    g_free(props);

    return (Py_ssize_t)n_props;
}

static PySequenceMethods _PyGProps_as_sequence = {
    (lenfunc) PyGProps_length,
    0,
    0,
    0,
    0,
    0,
    0
};

PYGLIB_DEFINE_TYPE("gobject.GPropsDescr", PyGPropsDescr_Type, PyObject);

static PyObject *
pyg_props_descr_descr_get(PyObject *self, PyObject *obj, PyObject *type)
{
    PyGProps *gprops;

    gprops = PyObject_GC_New(PyGProps, &PyGProps_Type);
    if (obj == NULL || obj == Py_None) {
        gprops->pygobject = NULL;
        gprops->gtype = pyg_type_from_object(type);
    } else {
        if (!PyObject_IsInstance(obj, (PyObject *) &PyGObject_Type)) {
            PyErr_SetString(PyExc_TypeError, "cannot use GObject property"
                            " descriptor on non-GObject instances");
            return NULL;
        }
        Py_INCREF(obj);
        gprops->pygobject = (PyGObject *) obj;
        gprops->gtype = pyg_type_from_object(obj);
    }
    return (PyObject *) gprops;
}
#endif

/**
 * hbgobject_register_class:
 * @type_name: not used ?
 * @gtype: the GType of the GObject subclass.
 * @static_bases: a tuple of Harbour class handles that are the bases of
 * this type
 *
 * This function is used to register a Harbour class as the wrapper for
 * a particular GObject subclass.
 */
HB_USHORT
hbgobject_register_class(const gchar *type_name,
			 GType gtype,
			 PHB_ITEM static_bases)
{
    //const char *s;
    //PHB_ITEM runtime_bases;
    PHB_ITEM bases;
    //int i;
    HB_USHORT type;

    if (static_bases) {
        bases = static_bases;
#if 0
        HB_USHORT hb_parent_type = hb_arrayGetNI(static_bases, 1);
          /* we start at index 2 because we want to skip the primary
           * base, otherwise we might get MRO conflict */
        for (i = 2; i <= hb_arrayLen(runtime_bases); ++i)
        {
            PHB_ITEM base = hb_arrayGetItemPtr(runtime_bases, i);
            HB_SIZE contains = hb_arrayScan(bases_list, base, NULL, NULL, TRUE);
            if (contains < 0)
                PyErr_Print();
            else if (!contains) {
                if (!PySequence_Contains(py_parent_type->tp_mro, base)) {
#if 0
                    g_message("Adding missing base %s to type %s",
                              ((PyTypeObject *)base)->tp_name, type->tp_name);
#endif
                    PyList_Append(bases_list, base);
                }
            }
        }
        bases = PySequence_Tuple(bases_list);
        Py_DECREF(bases_list);
        Py_DECREF(runtime_bases);
#endif
    } else
        bases = hbg_type_get_bases(gtype);

    //pygobject_inherit_slots(type, bases, TRUE);

    type = hbgi_hb_clsNew(type_name, HBGOBJECT_IVAR_COUNT, bases);
    if (type == 0) {
	g_warning ("couldn't make the type `%s' ready", type_name);
	return 0;
    }

    if (gtype) {
        HBGOBJECT_REGISTER_GTYPE(type, type_name, gtype);

	/* stash a handle of the Harbour class with the GType */
	g_type_set_qdata(gtype, hbgobject_class_key, GUINT_TO_POINTER(type));
    }

    /* set up __doc__ descriptor on type */
    /*PyDict_SetItemString(type->tp_dict, "__doc__",
			 pyg_object_descr_doc_get());

    PyDict_SetItemString(dict, (char *)class_name, (PyObject *)type);*/
    return type;
}

static void
hbg_toggle_notify (gpointer data, GObject *object, gboolean is_last_ref)
{
    PHB_ITEM self = (PHB_ITEM)data;
    /*PyGILState_STATE state;

    state = pyglib_gil_state_ensure();*/

    if (is_last_ref)
	hb_itemRelease(self);
    else {
        g_object_remove_toggle_ref(object, hbg_toggle_notify, self);
        g_object_add_toggle_ref(object, hbg_toggle_notify, hb_itemNew(self));
    }
    //pyglib_gil_state_release(state);
}

  /* Called when the inst_dict is first created; switches the
     reference counting strategy to start using toggle ref to keep the
     wrapper alive while the GObject lives.  In contrast, while
     inst_dict was NULL the Harbour wrapper is allowed to die at
     will and is recreated on demand. */
static inline void
hbgobject_switch_to_toggle_ref(PHB_ITEM self)
{
    GObject *gobject = hbgobject_get(self);
    guint32 flags;

    g_assert(gobject->ref_count >= 1);

    flags = GPOINTER_TO_UINT(g_object_get_qdata(gobject, hbgobject_private_flags_key));
    if (flags & HBGOBJECT_USING_TOGGLE_REF)
        return; /* already using toggle ref */
    g_object_set_qdata(gobject, hbgobject_private_flags_key, GUINT_TO_POINTER(flags | HBGOBJECT_USING_TOGGLE_REF));

      /* Note that add_toggle_ref will never immediately call back into
         hbg_toggle_notify */
    g_object_add_toggle_ref(gobject, hbg_toggle_notify, hb_itemNew(self));
    g_object_unref(gobject);
}


/**
 * hbgobject_register_wrapper:
 * @self: the wrapper instance
 *
 * In the constructor of HbGObject wrappers, this function should be
 * called after setting the obj member.  It will tie the wrapper
 * instance to the GObject so that the same wrapper instance will
 * always be used for this GObject instance.
 */
void
hbgobject_register_wrapper(PHB_ITEM self)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(hb_clsIsParent(hb_objGetClass(self), "GOBJECT"));

    hbgobject_sink(hbgobject_get(self));
    g_assert(hbgobject_get(self)->ref_count >= 1);
      /* save wrapper pointer so we can access it later */
    g_object_set_qdata_full(hbgobject_get(self), hbgobject_wrapper_key, self, NULL);
    if (0/*gself->inst_dict*/)
        hbgobject_switch_to_toggle_ref(self);
}

static PHB_ITEM
hbg_type_get_bases(GType gtype)
{
    GType *interfaces, parent_type, interface_type;
    guint n_interfaces;
    HB_USHORT hb_parent_type, hb_interface_type;
    PHB_ITEM bases;
    guint i;

    if (G_UNLIKELY(gtype == G_TYPE_OBJECT))
        return NULL;

    /* Lookup the parent type */
    parent_type = g_type_parent(gtype);
    hb_parent_type = hbgobject_lookup_class(parent_type);
    interfaces = g_type_interfaces(gtype, &n_interfaces);
    bases = hb_itemArrayNew(n_interfaces + 1);
    /* We will always put the parent at the first position in bases */
    hb_arraySetNL(bases, 1, hb_parent_type);

    /* And traverse interfaces */
    if (n_interfaces) {
	for (i = 0; i < n_interfaces; i++) {
	    interface_type = interfaces[i];
	    hb_interface_type = hbgobject_lookup_class(interface_type);
	    hb_arraySetNL(bases, 1 + i + 1, hb_interface_type);
	}
    }
    g_free(interfaces);
    return bases;
}

/**
 * hbgobject_new_with_interfaces
 * @gtype: the GType of the GObject subclass.
 *
 * Creates a new class from the given GType with interfaces attached in
 * bases.
 *
 * Returns: a class handle for the new type or NULL if it couldn't be created
 */
static HB_USHORT
hbgobject_new_with_interfaces(GType gtype)
{
    //PyGILState_STATE state;
    HB_USHORT type;
    HB_USHORT hb_parent_type;
    PHB_ITEM bases;
    //PHB_ITEM modules, module;
    gchar *type_name, *mod_name, *gtype_name;

    //state = pyglib_gil_state_ensure();

    bases = hbg_type_get_bases(gtype);
    hb_parent_type = hb_itemGetNL(hb_arrayGetItemPtr(bases, 1));

    /* set up __doc__ descriptor on type */
    //hb_arraySetC(dict, HBGOBJECT_IVAR_DOC, hbg_object_descr_doc_get());

    /* generate the hbgtk module name and extract the base type name */
    gtype_name = (gchar*)g_type_name(gtype);
    if (g_str_has_prefix(gtype_name, "Gtk")) {
	mod_name = "gtk";
	gtype_name += 3;
	type_name = g_strconcat(mod_name, ":", gtype_name, NULL);
    } else if (g_str_has_prefix(gtype_name, "Gdk")) {
	mod_name = "gtk:gdk";
	gtype_name += 3;
	type_name = g_strconcat(mod_name, ":", gtype_name, NULL);
    } else if (g_str_has_prefix(gtype_name, "Atk")) {
	mod_name = "atk";
	gtype_name += 3;
	type_name = g_strconcat(mod_name, ":", gtype_name, NULL);
    } else if (g_str_has_prefix(gtype_name, "Pango")) {
	mod_name = "pango";
	gtype_name += 5;
	type_name = g_strconcat(mod_name, ":", gtype_name, NULL);
    } else {
	mod_name = "__main__";
	type_name = g_strconcat(mod_name, ":", gtype_name, NULL);
    }

    type = hbgi_hb_clsNew(type_name, HBGI_IVAR_COUNT, bases);
    g_free(type_name);

    HBGOBJECT_REGISTER_GTYPE(type, type_name, gtype);

    if (type == 0) {
	/*PyErr_Print();
        pyglib_gil_state_release(state);*/
	return 0;
    }

#if 0
      /* Workaround python tp_(get|set)attr slot inheritance bug.
       * Fixes bug #144135. */
    if (!type->tp_getattr && py_parent_type->tp_getattr) {
        type->tp_getattro = NULL;
        type->tp_getattr = py_parent_type->tp_getattr;
    }
    if (!type->tp_setattr && py_parent_type->tp_setattr) {
        type->tp_setattro = NULL;
        type->tp_setattr = py_parent_type->tp_setattr;
    }
      /* override more python stupid hacks behind our back */
    type->tp_dealloc = py_parent_type->tp_dealloc;
    type->tp_alloc = py_parent_type->tp_alloc;
    type->tp_free = py_parent_type->tp_free;
    type->tp_traverse = py_parent_type->tp_traverse;
    type->tp_clear = py_parent_type->tp_clear;
#endif

    //hbgobject_inherit_slots(type, bases, FALSE);

#if 0
    if (PyType_Ready(type) < 0) {
	g_warning ("couldn't make the type `%s' ready", type->tp_name);
        pyglib_gil_state_release(state);
	return NULL;
    }

    /* insert type name in module dict */
    modules = PyImport_GetModuleDict();
    if ((module = PyDict_GetItemString(modules, mod_name)) != NULL) {
        if (PyObject_SetAttrString(module, gtype_name, (PyObject *)type) < 0)
            PyErr_Clear();
    }
#endif
    /* stash harbour class handle with the GType */
    g_type_set_qdata(gtype, hbgobject_class_key, GUINT_TO_POINTER(type));

    //pyglib_gil_state_release(state);

    return type;
}

#if 0
/* Pick appropriate value for given slot (at slot_offset inside
 * PyTypeObject structure).  It must be a pointer, e.g. a pointer to a
 * function.  We use the following heuristic:
 *
 * - Scan all types listed as bases of the type.
 * - If for exactly one base type slot value is non-NULL and
 *   different from that of 'object' and 'GObject', set current type
 *   slot into that value.
 * - Otherwise (if there is more than one such base type or none at
 *   all) don't touch it and live with Python default.
 *
 * The intention here is to propagate slot from custom wrappers to
 * wrappers created at runtime when appropriate.  We prefer to be on
 * the safe side, so if there is potential collision (more than one
 * custom slot value), we discard custom overrides altogether.
 *
 * When registering type with pygobject_register_class(), i.e. a type
 * that has been manually created (likely with Codegen help),
 * `check_for_present' should be set to TRUE.  In this case, the
 * function will never overwrite any non-NULL slots already present in
 * the type.  If `check_for_present' is FALSE, such non-NULL slots are
 * though to be set by Python interpreter and so will be overwritten
 * if heuristic above says so.
 */
static void
pygobject_inherit_slots(PyTypeObject *type, PyObject *bases, gboolean check_for_present)
{
    static int slot_offsets[] = { offsetof(PyTypeObject, tp_richcompare),
#if PY_VERSION_HEX < 0x03000000
                                  offsetof(PyTypeObject, tp_compare),
#endif
                                  offsetof(PyTypeObject, tp_richcompare),
                                  offsetof(PyTypeObject, tp_hash),
                                  offsetof(PyTypeObject, tp_iter),
                                  offsetof(PyTypeObject, tp_repr),
                                  offsetof(PyTypeObject, tp_str),
                                  offsetof(PyTypeObject, tp_print) };
    int i;

    /* Happens when registering gobject.GObject itself, at least. */
    if (!bases)
	return;

    for (i = 0; i < G_N_ELEMENTS(slot_offsets); ++i)
	pygobject_find_slot_for(type, bases, slot_offsets[i], check_for_present);
}

static void
pygobject_find_slot_for(PyTypeObject *type, PyObject *bases, int slot_offset,
			gboolean check_for_present)
{
#define TYPE_SLOT(type)  (* (void **) (((char *) (type)) + slot_offset))

    void *found_slot = NULL;
    int num_bases = PyTuple_Size(bases);
    int i;

    if (check_for_present && TYPE_SLOT(type) != NULL) {
	/* We are requested to check if there is any custom slot value
	 * in this type already and there actually is.  Don't
	 * overwrite it.
	 */
	return;
    }

    for (i = 0; i < num_bases; ++i) {
	PyTypeObject *base_type = (PyTypeObject *) PyTuple_GetItem(bases, i);
	void *slot = TYPE_SLOT(base_type);

	if (slot == NULL)
	    continue;
	if (slot == TYPE_SLOT(&PyGObject_Type) ||
	    slot == TYPE_SLOT(&PyBaseObject_Type))
	    continue;

	if (found_slot != NULL && found_slot != slot) {
	    /* We have a conflict: more than one base use different
	     * custom slots.  To be on the safe side, we bail out.
	     */
	    return;
	}

	found_slot = slot;
    }

    /* Only perform the final assignment if at least one base has a
     * custom value.  Otherwise just leave this type's slot untouched.
     */
    if (found_slot != NULL)
	TYPE_SLOT(type) = found_slot;

#undef TYPE_SLOT
}
#endif

/**
 * hbgobject_lookup_class:
 * @gtype: the GType of the GObject subclass.
 *
 * This function looks up the wrapper class used to represent
 * instances of a GObject represented by @gtype.  If no wrapper class
 * or interface has been registered for the given GType, then a new
 * type will be created.
 *
 * Returns: The wrapper class for the GObject or NULL if the
 *          GType has no registered type and a new type couldn't be created
 */
HB_USHORT
hbgobject_lookup_class(GType gtype)
{
    HB_USHORT hbclass;

    if (gtype == G_TYPE_INTERFACE)
	return HbGInterface_Type;

    hbclass = hbg_type_get_custom(g_type_name(gtype));
    if (hbclass)
	return hbclass;

    hbclass = GPOINTER_TO_UINT(g_type_get_qdata(gtype, hbgobject_class_key));
    if (hbclass == 0) {
	hbclass = GPOINTER_TO_UINT(g_type_get_qdata(gtype, hbginterface_type_key));

    if (hbclass == 0)
        hbclass = hbgi_type_import_by_g_type(gtype);

	if (hbclass == 0) {
	    hbclass = hbgobject_new_with_interfaces(gtype);
	    g_type_set_qdata(gtype, hbginterface_type_key, GUINT_TO_POINTER(hbclass));
	}
    }
    
    return hbclass;
}


/**
 * hbgobject_new_full:
 * @obj: a GObject instance.
 * @sink: whether to sink any floating reference found on the GObject. DEPRECATED.
 * @g_class: the GObjectClass
 *
 * This function gets a reference to a wrapper for the given GObject
 * instance.  If a wrapper has already been created, a new reference
 * to that wrapper will be returned.  Otherwise, a wrapper instance
 * will be created.
 *
 * Returns: a reference to the wrapper for the GObject.
 */
PHB_ITEM
hbgobject_new_full(GObject *obj, gboolean sink, gpointer g_class)
{
    PHB_ITEM self;

    HB_SYMBOL_UNUSED(sink);

    if (obj == NULL) {
        return hb_itemNew(NULL);
    }

    /* we already have a wrapper for this object -- return it. */
    self = (PHB_ITEM)g_object_get_qdata(obj, hbgobject_wrapper_key);
    if (self != NULL) {
	self = hb_itemNew(self);
    } else {
	/* create wrapper */
        HbGObjectData *inst_data = hbg_object_peek_inst_data(obj);
        HB_USHORT uiClass;
        if (inst_data)
            uiClass = inst_data->type;
        else {
            if (g_class)
                uiClass = hbgobject_lookup_class(G_OBJECT_CLASS_TYPE(g_class));
            else
                uiClass = hbgobject_lookup_class(G_OBJECT_TYPE(obj));
        }
        g_assert(uiClass);

        /* need to bump type refcount if created with
           hbgobject_new_with_interfaces(). fixes bug #141042 */
        /*if (tp->tp_flags & Py_TPFLAGS_HEAPTYPE)
            Py_INCREF(tp);*/
	self = hbgi_hb_clsInst(uiClass);
	if (self == NULL)
	    return NULL;
        self = hb_itemNew(self);
        hb_arraySetPtr(self, HBGI_IVAR_GOBJECT, obj);
	g_object_ref(obj);
        /*g_print("Registering wrapper %p of type %s for GType %s\n",
                self, hb_itemTypeStr(self), g_type_name(g_class ? G_OBJECT_CLASS_TYPE(g_class) : G_OBJECT_TYPE(obj)));*/
	hbgobject_register_wrapper(hb_itemNew(self));
	//PyObject_GC_Track((PyObject *)self);
    }

    return self;
}


PHB_ITEM
hbgobject_new(GObject *obj)
{
    return hbgobject_new_full(obj, TRUE, NULL);
}

PHB_ITEM
hbgobject_new_sunk(GObject *obj)
{
    g_object_set_qdata (obj, hbgobject_ref_sunk_key, GINT_TO_POINTER (1));
    return hbgobject_new_full(obj, TRUE, NULL);
}

static void
hbgobject_unwatch_closure(gpointer data, GClosure *closure)
{
    HbGObjectData *inst_data = data;

    inst_data->closures = g_slist_remove (inst_data->closures, closure);
}

/**
 * hbgobject_watch_closure:
 * @self: a GObject wrapper instance
 * @closure: a GClosure to watch
 *
 * Adds a closure to the list of watched closures for the wrapper.
 * The closure must be one returned by hbg_closure_new().  When the
 * cycle GC traverses the wrapper instance, it will enumerate the
 * references to Harbour objects stored in watched closures.  If the
 * cycle GC tells the wrapper to clear itself, the watched closures
 * will be invalidated.
 */
void
hbgobject_watch_closure(PHB_ITEM self, GClosure *closure)
{
    HbGObjectData *data;

    g_return_if_fail(self != NULL);
    g_return_if_fail(hb_clsIsParent(hb_objGetClass(self), "GOBJECT"));
    g_return_if_fail(closure != NULL);

    data = hbgobject_get_inst_data(self);
    g_return_if_fail(g_slist_find(data->closures, closure) == NULL);
    data->closures = g_slist_prepend(data->closures, closure);
    g_closure_add_invalidate_notifier(closure, data, hbgobject_unwatch_closure);
}

#if 0
/* -------------- PyGObject behaviour ----------------- */

PYGLIB_DEFINE_TYPE("gobject.GObject", PyGObject_Type, PyGObject);

static void
pygobject_dealloc(PyGObject *self)
{
    /* Untrack must be done first. This is because followup calls such as
     * ClearWeakRefs could call into Python and cause new allocations to
     * happen, which could in turn could trigger the garbage collector,
     * which would then get confused as it is tracking this half-deallocated
     * object. */
    PyObject_GC_UnTrack((PyObject *)self);

    PyObject_ClearWeakRefs((PyObject *)self);
      /* this forces inst_data->type to be updated, which could prove
       * important if a new wrapper has to be created and it is of a
       * unregistered type */
    pygobject_get_inst_data(self);
    pygobject_clear(self);
    /* the following causes problems with subclassed types */
    /* Py_TYPE(self)->tp_free((PyObject *)self); */
    PyObject_GC_Del(self);
}

static PyObject*
pygobject_richcompare(PyObject *self, PyObject *other, int op)
{
    int isinst;

    isinst = PyObject_IsInstance(self, (PyObject*)&PyGObject_Type);
    if (isinst == -1)
        return NULL;
    if (!isinst) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }
    isinst = PyObject_IsInstance(other, (PyObject*)&PyGObject_Type);
    if (isinst == -1)
        return NULL;
    if (!isinst) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    return _pyglib_generic_ptr_richcompare(((PyGObject*)self)->obj,
                                           ((PyGObject*)other)->obj,
                                           op);
}

static long
pygobject_hash(PyGObject *self)
{
    return (long)self->obj;
}

static PyObject *
pygobject_repr(PyGObject *self)
{
    gchar buf[256];

    g_snprintf(buf, sizeof(buf),
	       "<%s object at 0x%lx (%s at 0x%lx)>",
	       Py_TYPE(self)->tp_name,
	       (long)self,
	       self->obj ? G_OBJECT_TYPE_NAME(self->obj) : "uninitialized",
               (long)self->obj);
    return PYGLIB_PyUnicode_FromString(buf);
}


static int
pygobject_traverse(PyGObject *self, visitproc visit, void *arg)
{
    int ret = 0;
    GSList *tmp;
    PyGObjectData *data = pygobject_get_inst_data(self);

    if (self->inst_dict) ret = visit(self->inst_dict, arg);
    if (ret != 0) return ret;

    if (data) {

        for (tmp = data->closures; tmp != NULL; tmp = tmp->next) {
            PyGClosure *closure = tmp->data;

            if (closure->callback) ret = visit(closure->callback, arg);
            if (ret != 0) return ret;

            if (closure->extra_args) ret = visit(closure->extra_args, arg);
            if (ret != 0) return ret;

            if (closure->swap_data) ret = visit(closure->swap_data, arg);
            if (ret != 0) return ret;
        }
    }
    return ret;
}
#endif

HB_FUNC_STATIC(hbgobject_clear)
{
    PHB_ITEM self = hb_stackSelfItem();
    GObject *obj = hbgobject_get(self);

    if (obj) {
        g_object_set_qdata_full(obj, hbgobject_wrapper_key, NULL, NULL);
        /*if (self->inst_dict) {
            g_object_remove_toggle_ref(self->obj, hbg_toggle_notify, self);
            self->private_flags.flags &= ~PYGOBJECT_USING_TOGGLE_REF;
        } else*/ {
            //pyg_begin_allow_threads;
            g_object_unref(obj);
            //pyg_end_allow_threads;
        }
        //self->obj = NULL;
    }
    //Py_CLEAR(self->inst_dict);
}

#if 0
static void
pygobject_free(PyObject *op)
{
    PyObject_GC_Del(op);
}

gboolean
pygobject_prepare_construct_properties(GObjectClass *class, PyObject *kwargs,
                                       guint *n_params, GParameter **params)
{
    *n_params = 0;
    *params = NULL;

    if (kwargs) {
        Py_ssize_t pos = 0;
        PyObject *key;
        PyObject *value;

        *params = g_new0(GParameter, PyDict_Size(kwargs));
        while (PyDict_Next(kwargs, &pos, &key, &value)) {
            GParamSpec *pspec;
            GParameter *param = &(*params)[*n_params];
            const gchar *key_str = PYGLIB_PyUnicode_AsString(key);

            pspec = g_object_class_find_property(class, key_str);
            if (!pspec) {
                PyErr_Format(PyExc_TypeError,
                             "gobject `%s' doesn't support property `%s'",
                             G_OBJECT_CLASS_NAME(class), key_str);
                return FALSE;
            }
            g_value_init(&param->value, G_PARAM_SPEC_VALUE_TYPE(pspec));
            if (pyg_param_gvalue_from_pyobject(&param->value, value, pspec) < 0) {
                PyErr_Format(PyExc_TypeError,
                             "could not convert value for property `%s' from %s to %s",
                             key_str, Py_TYPE(value)->tp_name,
                             g_type_name(G_PARAM_SPEC_VALUE_TYPE(pspec)));
                return FALSE;
            }
            param->name = g_strdup(key_str);
            ++(*n_params);
        }
    }
    return TRUE;
}

/* ---------------- PyGObject methods ----------------- */

static int
pygobject_init(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    GType object_type;
    guint n_params = 0, i;
    GParameter *params = NULL;
    GObjectClass *class;

    if (!PyArg_ParseTuple(args, ":GObject.__init__", &object_type))
	return -1;

    object_type = pyg_type_from_object((PyObject *)self);
    if (!object_type)
	return -1;

    if (G_TYPE_IS_ABSTRACT(object_type)) {
	PyErr_Format(PyExc_TypeError, "cannot create instance of abstract "
		     "(non-instantiable) type `%s'", g_type_name(object_type));
	return -1;
    }

    if ((class = g_type_class_ref (object_type)) == NULL) {
	PyErr_SetString(PyExc_TypeError,
			"could not get a reference to type class");
	return -1;
    }

    if (!pygobject_prepare_construct_properties (class, kwargs, &n_params, &params))
        goto cleanup;

    if (pygobject_constructv(self, n_params, params))
	PyErr_SetString(PyExc_RuntimeError, "could not create object");
	   
 cleanup:
    for (i = 0; i < n_params; i++) {
	g_free((gchar *) params[i].name);
	g_value_unset(&params[i].value);
    }
    g_free(params);
    g_type_class_unref(class);
    
    return (self->obj) ? 0 : -1;
}

static PyObject *
pygobject__gobject_init__(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    if (pygobject_init(self, args, kwargs) < 0)
	return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}
#endif

#define CHECK_GOBJECT(self) \
    if (!G_IS_OBJECT(hbg_object_get(self))) { \
        gchar *msg = g_strdup_printf("object at %p of type %s is not initialized", self, hb_clsName(hb_objGetClass(self))); \
        hb_errRT_BASE_SubstR(EG_DATATYPE, 50104, msg, "hbgobject", HB_ERR_ARGS_BASEPARAMS); \
        g_free(msg); \
	return; \
    }

HB_FUNC_STATIC(hbgobject_get_property)
{
    PHB_ITEM self = hb_stackSelfItem();
    const char *param_name = hb_parc(1);
    GParamSpec *pspec;
    GValue value = { 0, };

    if (hb_pcount() != 1 || !param_name) {
        hb_errRT_BASE_SubstR(EG_ARG, 50113, "GObject.get_property single argument must be string", "hbgobject", HB_ERR_ARGS_BASEPARAMS);
        return;
    }

    CHECK_GOBJECT(self);

    pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(hbgobject_get(self)),
					 param_name);
    if (!pspec) {
        gchar *msg = g_strdup_printf("object of type `%s' does not have property `%s'",
		     g_type_name(G_OBJECT_TYPE(hbg_object_get(self))), param_name);
        hb_errRT_BASE_SubstR(EG_ARG, 50114, msg, "hbgobject", HB_ERR_ARGS_BASEPARAMS);
        g_free(msg);
	return;
    }
    if (!(pspec->flags & G_PARAM_READABLE)) {
        gchar *msg = g_strdup_printf("property %s is not readable",
		     param_name);
        hb_errRT_BASE_SubstR(EG_ARG, 50115, msg, "hbgobject", HB_ERR_ARGS_BASEPARAMS);
        g_free(msg);
	return;
    }
    g_value_init(&value, G_PARAM_SPEC_VALUE_TYPE(pspec));
    //pyg_begin_allow_threads;
    g_object_get_property(hbgobject_get(self), param_name, &value);
    //pyg_end_allow_threads;
    hbgi_hb_itemReturnRelease(hbg_param_gvalue_as_hbitem(&value, TRUE, pspec));
    g_value_unset(&value);
}

HB_FUNC_STATIC(hbgobject_get_properties)
{
    PHB_ITEM self = hb_stackSelfItem();
    GObjectClass *class;
    int len, i;
    PHB_ITEM tuple;
    GObject *gobject = hbg_object_get(self);

    if ((len = hb_pcount()) < 1) {
        hb_errRT_BASE_SubstR(EG_ARG, 50117, "GObject.get_properties requires at least one argument", "hbgobject", HB_ERR_ARGS_BASEPARAMS);
        return;
    }

    tuple = hb_itemArrayNew(len);
    class = G_OBJECT_GET_CLASS(gobject);
    for (i = 1; i <= len; i++) {
        const char *property_name = hb_parc(i);
        GParamSpec *pspec;
        GValue value = { 0, };
        PHB_ITEM item;

        if (!HB_ISCHAR(i)) {
            hb_errRT_BASE_SubstR(EG_ARG, 50118, "GObject.get_properties: Expected string argument for property", "hbgobject", HB_ERR_ARGS_BASEPARAMS);
            return;
        }

        pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(gobject),
					 property_name);
        if (!pspec) {
            gchar *msg = g_strdup_printf(
		         "object of type `%s' does not have property `%s'",
		         g_type_name(G_OBJECT_TYPE(gobject)), property_name);
            hb_errRT_BASE_SubstR(EG_ARG, 50119, msg, "hbgobject", HB_ERR_ARGS_BASEPARAMS);
    	    return;
        }
        if (!(pspec->flags & G_PARAM_READABLE)) {
            gchar *msg = g_strdup_printf("property %s is not readable",
                         property_name);
            hb_errRT_BASE_SubstR(EG_ARG, 50115, msg, "hbgobject", HB_ERR_ARGS_BASEPARAMS);
            g_free(msg);
            return;
        }
        g_value_init(&value, G_PARAM_SPEC_VALUE_TYPE(pspec));

        //pyg_begin_allow_threads;
        g_object_get_property(gobject, property_name, &value);
        //pyg_end_allow_threads;

        item = hbg_value_as_hbitem(&value, TRUE);
        hb_arraySetForward(tuple, i, item);
        hb_itemRelease(item);

        g_value_unset(&value);
    }

    hb_itemReturnRelease(tuple);
}

#if 0
static PyObject *
pygobject_set_property(PyGObject *self, PyObject *args)
{
    gchar *param_name;
    GParamSpec *pspec;
    PyObject *pvalue;

    if (!PyArg_ParseTuple(args, "sO:GObject.set_property", &param_name,
			  &pvalue))
	return NULL;
    
    CHECK_GOBJECT(self);
    
    pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(self->obj),
					 param_name);
    if (!pspec) {
	PyErr_Format(PyExc_TypeError,
		     "object of type `%s' does not have property `%s'",
		     g_type_name(G_OBJECT_TYPE(self->obj)), param_name);
	return NULL;
    }
    
    if (!set_property_from_pspec(self->obj, param_name, pspec, pvalue))
	return NULL;
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygobject_set_properties(PyGObject *self, PyObject *args, PyObject *kwargs)
{    
    GObjectClass    *class;
    Py_ssize_t      pos;
    PyObject        *value;
    PyObject        *key;
    PyObject        *result = NULL;

    CHECK_GOBJECT(self);

    class = G_OBJECT_GET_CLASS(self->obj);
    
    g_object_freeze_notify (G_OBJECT(self->obj));
    pos = 0;

    while (kwargs && PyDict_Next (kwargs, &pos, &key, &value)) {
	gchar *key_str = PYGLIB_PyUnicode_AsString(key);
	GParamSpec *pspec;

	pspec = g_object_class_find_property(class, key_str);
	if (!pspec) {
	    gchar buf[512];

	    g_snprintf(buf, sizeof(buf),
		       "object `%s' doesn't support property `%s'",
		       g_type_name(G_OBJECT_TYPE(self->obj)), key_str);
	    PyErr_SetString(PyExc_TypeError, buf);
	    goto exit;
	}

	if (!set_property_from_pspec(G_OBJECT(self->obj), key_str, pspec, value))
	    goto exit;
    }

    result = Py_None;

 exit:
    g_object_thaw_notify (G_OBJECT(self->obj));
    Py_XINCREF(result);
    return result;
}
#endif

HB_FUNC_STATIC(hbgobject_freeze_notify)
{
    PHB_ITEM self = hb_stackSelfItem();

    if (hb_pcount() > 0)
    {
	hb_ret();
        return;
    }

    CHECK_GOBJECT(self);

    g_object_freeze_notify(hbg_object_get(self));
    hb_ret();
}

HB_FUNC_STATIC(hbgobject_notify)
{
    PHB_ITEM self = hb_stackSelfItem();
    const char *property_name = hb_parc(1);

    if (hb_pcount() != 1 || !property_name)
    {
	hb_ret();
        return;
    }

    CHECK_GOBJECT(self);

    g_object_notify(hbg_object_get(self), property_name);
    hb_ret();
}

HB_FUNC_STATIC(hbgobject_thaw_notify)
{
    PHB_ITEM self = hb_stackSelfItem();

    if (hb_pcount() != 0)
    {
	hb_ret();
        return;
    }

    CHECK_GOBJECT(self);

    g_object_thaw_notify(hbg_object_get(self));
    hb_ret();
}

HB_FUNC_STATIC(hbgobject_get_data)
{
    PHB_ITEM self = hb_stackSelfItem();
    const char *key = hb_parc(1);
    GQuark quark;
    PHB_ITEM data;

    if (hb_pcount() != 1 || !key)
    {
	hb_ret();
        return;
    }

    CHECK_GOBJECT(self);

    quark = g_quark_from_string(key);
    data = g_object_get_qdata(hbg_object_get(self), quark);
    if (!data)
    {
       hb_ret();
    }
    else
    {
       hb_itemReturn(data);
    }
}

HB_FUNC_STATIC(hbgobject_set_data)
{
    PHB_ITEM self = hb_stackSelfItem();
    const char *key = hb_parc(1);
    GQuark quark;
    PHB_ITEM data;

    if (hb_pcount() != 2 || !key)
    {
	hb_ret();
        return;
    }

    CHECK_GOBJECT(self);

    quark = g_quark_from_string(key);
    data = hb_itemParam(2);
    g_object_set_qdata_full(hbg_object_get(self), quark, data, hbg_destroy_notify);
    hb_ret();
}

static void
hbgobject_connect_common(gboolean after, gboolean with_object)
{
    PHB_ITEM self, callback, extra_args;
    const char *name;
    guint len, arg_pos;
    GObject *object = NULL;
    guint sigid;
    gulong handlerid;
    GQuark detail = 0;
    GClosure *closure;
    GObject *gobject;

    len = hb_pcount();
    if (with_object) {
        arg_pos = 4;
        if (len < arg_pos - 1) {
            hb_errRT_BASE_SubstR(EG_ARG, 50101, "GObject.connect_object requires at least 3 arguments", "hbgobject", HB_ERR_ARGS_BASEPARAMS);
            return;
        }
    } else {
        arg_pos = 3;
        if (len < arg_pos - 1) {
            hb_errRT_BASE_SubstR(EG_ARG, 50101, "GObject.connect requires at least 2 arguments", "hbgobject", HB_ERR_ARGS_BASEPARAMS);
            return;
        }
    }

    name = hb_parc(1);
    if (!name)
    {
        hb_errRT_BASE_SubstR(EG_ARG, 50102, "GObject.connect first argument must be string", "hbgobject", HB_ERR_ARGS_BASEPARAMS);
        return;
    }
    callback = hb_itemParam(2);
    if (!(HB_IS_SYMBOL(callback) || HB_IS_BLOCK(callback)))
    {
        hb_itemRelease(callback);
        hb_errRT_BASE_SubstR(EG_ARG, 50103, "GObject.connect second argument must be callable", "hbgobject", HB_ERR_ARGS_BASEPARAMS);
        return;
    }
    if (with_object) {
        object = hb_itemParam(3);
    }

    self = hb_stackSelfItem();
    CHECK_GOBJECT(self);

    gobject = hbg_object_get(self);
    if (!g_signal_parse_name(name, G_OBJECT_TYPE(gobject),
			     &sigid, &detail, TRUE)) {
        gchar *msg = g_strdup_printf("%s: unknown signal name: %s", hb_clsName(hb_objGetClass(self)), name);
        hb_errRT_BASE_SubstR(EG_ARG, 50105, msg, "hbgobject", HB_ERR_ARGS_BASEPARAMS);
        g_free(msg);
	return;
    }

    extra_args = hbgi_hb_paramSlice(arg_pos, len + 1);

    closure = hbgi_signal_closure_new(self, name, callback, extra_args, object);
    if (closure == NULL)
        closure = hbg_closure_new(callback, extra_args, object);

    hbgobject_watch_closure(self, closure);
    hb_itemRelease(extra_args);
    hb_itemRelease(callback);
    if (object) {
        hb_itemRelease(object);
    }
    handlerid = g_signal_connect_closure_by_id(gobject, sigid, detail,
					       closure, after);
    hb_retnll(handlerid);
}

HB_FUNC_STATIC(hbgobject_connect)
{
    hbgobject_connect_common(FALSE, FALSE);
}

HB_FUNC_STATIC(hbgobject_connect_after)
{
    hbgobject_connect_common(TRUE, FALSE);
}

HB_FUNC_STATIC(hbgobject_connect_object)
{
    hbgobject_connect_common(FALSE, TRUE);
}

HB_FUNC_STATIC(hbgobject_connect_object_after)
{
    hbgobject_connect_common(TRUE, TRUE);
}

HB_FUNC_STATIC(hbgobject_disconnect)
{
    PHB_ITEM self = hb_stackSelfItem();
    gulong handler_id;

    if (hb_pcount() != 1 || !HB_ISNUM(1))
    {
        hb_errRT_BASE_SubstR(EG_ARG, 50107, "GObject.disconnect single argument must be number", "hbgobject", HB_ERR_ARGS_BASEPARAMS);
        return;
    }
    handler_id = hb_parnll(1);

    CHECK_GOBJECT(self);

    g_signal_handler_disconnect(hbg_object_get(self), handler_id);
    hb_ret();
}

HB_FUNC_STATIC(hbgobject_handler_is_connected)
{
    PHB_ITEM self = hb_stackSelfItem();
    gulong handler_id;

    if (hb_pcount() != 1 || !HB_ISNUM(1))
    {
        hb_errRT_BASE_SubstR(EG_ARG, 50108, "GObject.handler_is_connected single argument must be number", "hbgobject", HB_ERR_ARGS_BASEPARAMS);
        return;
    }
    handler_id = hb_parnll(1);

    CHECK_GOBJECT(self);

    hb_retl(g_signal_handler_is_connected(hbg_object_get(self), handler_id));
}

HB_FUNC_STATIC(hbgobject_handler_block)
{
    PHB_ITEM self = hb_stackSelfItem();
    gulong handler_id;

    if (hb_pcount() != 1 || !HB_ISNUM(1))
    {
        hb_errRT_BASE_SubstR(EG_ARG, 50109, "GObject.handler_block single argument must be number", "hbgobject", HB_ERR_ARGS_BASEPARAMS);
        return;
    }
    handler_id = hb_parnll(1);

    CHECK_GOBJECT(self);

    g_signal_handler_block(hbg_object_get(self), handler_id);
    hb_ret();
}

HB_FUNC_STATIC(hbgobject_handler_unblock)
{
    PHB_ITEM self = hb_stackSelfItem();
    gulong handler_id;

    if (hb_pcount() != 1 || !HB_ISNUM(1))
    {
        hb_errRT_BASE_SubstR(EG_ARG, 50110, "GObject.handler_unblock single argument must be number", "hbgobject", HB_ERR_ARGS_BASEPARAMS);
        return;
    }
    handler_id = hb_parnll(1);

    CHECK_GOBJECT(self);

    g_signal_handler_unblock(hbg_object_get(self), handler_id);
    hb_ret();
}

#if 0
static PyObject *
pygobject_emit(PyGObject *self, PyObject *args)
{
    guint signal_id, i;
    Py_ssize_t len;
    GQuark detail;
    PyObject *first, *py_ret;
    gchar *name;
    GSignalQuery query;
    GValue *params, ret = { 0, };
    
    len = PyTuple_Size(args);
    if (len < 1) {
	PyErr_SetString(PyExc_TypeError,"GObject.emit needs at least one arg");
	return NULL;
    }
    first = PySequence_GetSlice(args, 0, 1);
    if (!PyArg_ParseTuple(first, "s:GObject.emit", &name)) {
	Py_DECREF(first);
	return NULL;
    }
    Py_DECREF(first);
    
    CHECK_GOBJECT(self);
    
    if (!g_signal_parse_name(name, G_OBJECT_TYPE(self->obj),
			     &signal_id, &detail, TRUE)) {
	PyErr_Format(PyExc_TypeError, "%s: unknown signal name: %s",
		     PYGLIB_PyUnicode_AsString(PyObject_Repr((PyObject*)self)),
		     name);
	return NULL;
    }
    g_signal_query(signal_id, &query);
    if (len != query.n_params + 1) {
	gchar buf[128];

	g_snprintf(buf, sizeof(buf),
		   "%d parameters needed for signal %s; %ld given",
		   query.n_params, name, (long int) (len - 1));
	PyErr_SetString(PyExc_TypeError, buf);
	return NULL;
    }

    params = g_new0(GValue, query.n_params + 1);
    g_value_init(&params[0], G_OBJECT_TYPE(self->obj));
    g_value_set_object(&params[0], G_OBJECT(self->obj));

    for (i = 0; i < query.n_params; i++)
	g_value_init(&params[i + 1],
		     query.param_types[i] & ~G_SIGNAL_TYPE_STATIC_SCOPE);
    for (i = 0; i < query.n_params; i++) {
	PyObject *item = PyTuple_GetItem(args, i+1);

	if (pyg_value_from_pyobject(&params[i+1], item) < 0) {
	    gchar buf[128];
	    g_snprintf(buf, sizeof(buf),
		       "could not convert type %s to %s required for parameter %d",
		       Py_TYPE(item)->tp_name,
		g_type_name(G_VALUE_TYPE(&params[i+1])), i);
	    PyErr_SetString(PyExc_TypeError, buf);

	    for (i = 0; i < query.n_params + 1; i++)
		g_value_unset(&params[i]);

	    g_free(params);
	    return NULL;
	}
    }    

    if (query.return_type != G_TYPE_NONE)
	g_value_init(&ret, query.return_type & ~G_SIGNAL_TYPE_STATIC_SCOPE);
    
    g_signal_emitv(params, signal_id, detail, &ret);

    for (i = 0; i < query.n_params + 1; i++)
	g_value_unset(&params[i]);
    
    g_free(params);
    if ((query.return_type & ~G_SIGNAL_TYPE_STATIC_SCOPE) != G_TYPE_NONE) {
	py_ret = pyg_value_as_pyobject(&ret, TRUE);
	g_value_unset(&ret);
    } else {
	Py_INCREF(Py_None);
	py_ret = Py_None;
    }

    return py_ret;
}
#endif

HB_FUNC_STATIC(hbgobject_stop_emission)
{
    PHB_ITEM self = hb_stackSelfItem();
    const char *signal = hb_parc(1);
    guint signal_id;
    GQuark detail;
    GObject *gobject;

    if (hb_pcount() != 1 || !signal)
    {
        hb_errRT_BASE_SubstR(EG_ARG, 50111, "GObject.stop_emission single argument must be string", "hbgobject", HB_ERR_ARGS_BASEPARAMS);
	return;
    }

    CHECK_GOBJECT(self);

    gobject = hbg_object_get(self);
    if (!g_signal_parse_name(signal, G_OBJECT_TYPE(gobject),
			     &signal_id, &detail, TRUE)) {
        gchar *msg = g_strdup_printf("%s: unknown signal name: %s", hb_clsName(hb_objGetClass(self)), signal);
        hb_errRT_BASE_SubstR(EG_ARG, 50112, msg, "hbgobject", HB_ERR_ARGS_BASEPARAMS);
        g_free(msg);
	return;
    }
    g_signal_stop_emission(gobject, signal_id, detail);
    hb_ret();
}

#if 0
static PyObject *
pygobject_chain_from_overridden(PyGObject *self, PyObject *args)
{
    GSignalInvocationHint *ihint;
    guint signal_id, i;
    Py_ssize_t len;
    PyObject *py_ret;
    const gchar *name;
    GSignalQuery query;
    GValue *params, ret = { 0, };
    
    CHECK_GOBJECT(self);
    
    ihint = g_signal_get_invocation_hint(self->obj);
    if (!ihint) {
	PyErr_SetString(PyExc_TypeError, "could not find signal invocation "
			"information for this object.");
	return NULL;
    }

    signal_id = ihint->signal_id;
    name = g_signal_name(signal_id);

    len = PyTuple_Size(args);
    if (signal_id == 0) {
	PyErr_SetString(PyExc_TypeError, "unknown signal name");
	return NULL;
    }
    g_signal_query(signal_id, &query);
    if (len != query.n_params) {
	gchar buf[128];

	g_snprintf(buf, sizeof(buf),
		   "%d parameters needed for signal %s; %ld given",
		   query.n_params, name, (long int) len);
	PyErr_SetString(PyExc_TypeError, buf);
	return NULL;
    }
    params = g_new0(GValue, query.n_params + 1);
    g_value_init(&params[0], G_OBJECT_TYPE(self->obj));
    g_value_set_object(&params[0], G_OBJECT(self->obj));

    for (i = 0; i < query.n_params; i++)
	g_value_init(&params[i + 1],
		     query.param_types[i] & ~G_SIGNAL_TYPE_STATIC_SCOPE);
    for (i = 0; i < query.n_params; i++) {
	PyObject *item = PyTuple_GetItem(args, i);

	if (pyg_boxed_check(item, (query.param_types[i] & ~G_SIGNAL_TYPE_STATIC_SCOPE))) {
	    g_value_set_static_boxed(&params[i+1], pyg_boxed_get(item, void));
	}
	else if (pyg_value_from_pyobject(&params[i+1], item) < 0) {
	    gchar buf[128];

	    g_snprintf(buf, sizeof(buf),
		       "could not convert type %s to %s required for parameter %d",
		       Py_TYPE(item)->tp_name,
		       g_type_name(G_VALUE_TYPE(&params[i+1])), i);
	    PyErr_SetString(PyExc_TypeError, buf);
	    for (i = 0; i < query.n_params + 1; i++)
		g_value_unset(&params[i]);
	    g_free(params);
	    return NULL;
	}
    }
    if (query.return_type != G_TYPE_NONE)
	g_value_init(&ret, query.return_type & ~G_SIGNAL_TYPE_STATIC_SCOPE);
    g_signal_chain_from_overridden(params, &ret);
    for (i = 0; i < query.n_params + 1; i++)
	g_value_unset(&params[i]);
    g_free(params);
    if (query.return_type != G_TYPE_NONE) {
	py_ret = pyg_value_as_pyobject(&ret, TRUE);
	g_value_unset(&ret);
    } else {
	Py_INCREF(Py_None);
	py_ret = Py_None;
    }
    return py_ret;
}


static PyObject *
pygobject_weak_ref(PyGObject *self, PyObject *args)
{
    int len;
    PyObject *callback = NULL, *user_data = NULL;
    PyObject *retval;

    CHECK_GOBJECT(self);

    if ((len = PySequence_Length(args)) >= 1) {
        callback = PySequence_ITEM(args, 0);
        user_data = PySequence_GetSlice(args, 1, len);
    }
    retval = pygobject_weak_ref_new(self->obj, callback, user_data);
    Py_XDECREF(callback);
    Py_XDECREF(user_data);
    return retval;
}


static PyObject *
pygobject_copy(PyGObject *self)
{
    PyErr_SetString(PyExc_TypeError,
		    "gobject.GObject descendants' instances are non-copyable");
    return NULL;
}

static PyObject *
pygobject_deepcopy(PyGObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_TypeError,
		    "gobject.GObject descendants' instances are non-copyable");
    return NULL;
}


static PyObject *
pygobject_disconnect_by_func(PyGObject *self, PyObject *args)
{
    PyObject *pyfunc = NULL;
    GClosure *closure = NULL;
    guint retval;
    
    CHECK_GOBJECT(self);

    if (!PyArg_ParseTuple(args, "O:GObject.disconnect_by_func", &pyfunc))
	return NULL;

    if (!PyCallable_Check(pyfunc)) {
	PyErr_SetString(PyExc_TypeError, "first argument must be callable");
	return NULL;
    }

    closure = gclosure_from_pyfunc(self, pyfunc);
    if (!closure) {
	PyErr_Format(PyExc_TypeError, "nothing connected to %s",
		     PYGLIB_PyUnicode_AsString(PyObject_Repr((PyObject*)pyfunc)));
	return NULL;
    }
    
    retval = g_signal_handlers_disconnect_matched(self->obj,
						  G_SIGNAL_MATCH_CLOSURE,
						  0, 0,
						  closure,
						  NULL, NULL);
    return PYGLIB_PyLong_FromLong(retval);
}

static PyObject *
pygobject_handler_block_by_func(PyGObject *self, PyObject *args)
{
    PyObject *pyfunc = NULL;
    GClosure *closure = NULL;
    guint retval;
    
    CHECK_GOBJECT(self);

    if (!PyArg_ParseTuple(args, "O:GObject.handler_block_by_func", &pyfunc))
	return NULL;

    if (!PyCallable_Check(pyfunc)) {
	PyErr_SetString(PyExc_TypeError, "first argument must be callable");
	return NULL;
    }

    closure = gclosure_from_pyfunc(self, pyfunc);
    if (!closure) {
	PyErr_Format(PyExc_TypeError, "nothing connected to %s",
		     PYGLIB_PyUnicode_AsString(PyObject_Repr((PyObject*)pyfunc)));
	return NULL;
    }
    
    retval = g_signal_handlers_block_matched(self->obj,
					     G_SIGNAL_MATCH_CLOSURE,
					     0, 0,
					     closure,
					     NULL, NULL);
    return PYGLIB_PyLong_FromLong(retval);
}

static PyObject *
pygobject_handler_unblock_by_func(PyGObject *self, PyObject *args)
{
    PyObject *pyfunc = NULL;
    GClosure *closure = NULL;
    guint retval;
    
    CHECK_GOBJECT(self);

    if (!PyArg_ParseTuple(args, "O:GObject.handler_unblock_by_func", &pyfunc))
	return NULL;

    if (!PyCallable_Check(pyfunc)) {
	PyErr_SetString(PyExc_TypeError, "first argument must be callable");
	return NULL;
    }

    closure = gclosure_from_pyfunc(self, pyfunc);
    if (!closure) {
	PyErr_Format(PyExc_TypeError, "nothing connected to %s",
		     PYGLIB_PyUnicode_AsString(PyObject_Repr((PyObject*)pyfunc)));
	return NULL;
    }
    
    retval = g_signal_handlers_unblock_matched(self->obj,
					       G_SIGNAL_MATCH_CLOSURE,
					       0, 0,
					       closure,
					       NULL, NULL);
    return PYGLIB_PyLong_FromLong(retval);
}


static PyObject *
pygobject_get_refcount(PyGObject *self, void *closure)
{
    return PYGLIB_PyLong_FromLong(self->obj->ref_count);
}

static int
pygobject_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    int res;
    PyGObject *gself = (PyGObject *) self;
    PyObject *inst_dict_before = gself->inst_dict;
      /* call parent type's setattro */
    res = PyGObject_Type.tp_base->tp_setattro(self, name, value);
    if (inst_dict_before == NULL && gself->inst_dict != NULL) {
        if (G_LIKELY(gself->obj))
            pygobject_switch_to_toggle_ref(gself);
    }
    return res;
}

static PyGetSetDef pygobject_getsets[] = {
    { "__grefcount__", (getter)pygobject_get_refcount, (setter)0, },
    { NULL, 0, 0 }
};

/* ------------------------------------ */
/* ****** GObject weak reference ****** */
/* ------------------------------------ */

typedef struct {
    PyObject_HEAD
    GObject *obj;
    PyObject *callback;
    PyObject *user_data;
    gboolean have_floating_ref;
} PyGObjectWeakRef;

PYGLIB_DEFINE_TYPE("gobject.GObjectWeakRef", PyGObjectWeakRef_Type, PyGObjectWeakRef);

static int
pygobject_weak_ref_traverse(PyGObjectWeakRef *self, visitproc visit, void *arg)
{
    if (self->callback && visit(self->callback, arg) < 0)
        return -1;
    if (self->user_data && visit(self->user_data, arg) < 0)
        return -1;
    return 0;
}

static void
pygobject_weak_ref_notify(PyGObjectWeakRef *self, GObject *dummy)
{
    self->obj = NULL;
    if (self->callback) {
        PyObject *retval;
        PyGILState_STATE state = pyglib_gil_state_ensure();
        retval = PyObject_Call(self->callback, self->user_data, NULL);
        if (retval) {
            if (retval != Py_None)
                PyErr_Format(PyExc_TypeError,
                             "GObject weak notify callback returned a value"
                             " of type %s, should return None",
                             Py_TYPE(retval)->tp_name);
            Py_DECREF(retval);
            PyErr_Print();
        } else
            PyErr_Print();
        Py_CLEAR(self->callback);
        Py_CLEAR(self->user_data);
        if (self->have_floating_ref) {
            self->have_floating_ref = FALSE;
            Py_DECREF((PyObject *) self);
        }
        pyglib_gil_state_release(state);
    }
}

static inline int
pygobject_weak_ref_clear(PyGObjectWeakRef *self)
{
    Py_CLEAR(self->callback);
    Py_CLEAR(self->user_data);
    if (self->obj) {
        g_object_weak_unref(self->obj, (GWeakNotify) pygobject_weak_ref_notify, self);
        self->obj = NULL;
    }
    return 0;
}

static void
pygobject_weak_ref_dealloc(PyGObjectWeakRef *self)
{
    PyObject_GC_UnTrack((PyObject *)self);
    pygobject_weak_ref_clear(self);
    PyObject_GC_Del(self);
}

static PyObject *
pygobject_weak_ref_new(GObject *obj, PyObject *callback, PyObject *user_data)
{
    PyGObjectWeakRef *self;

    self = PyObject_GC_New(PyGObjectWeakRef, &PyGObjectWeakRef_Type);
    self->callback = callback;
    self->user_data = user_data;
    Py_XINCREF(self->callback);
    Py_XINCREF(self->user_data);
    self->obj = obj;
    g_object_weak_ref(self->obj, (GWeakNotify) pygobject_weak_ref_notify, self);
    if (callback != NULL) {
          /* when we have a callback, we should INCREF the weakref
           * object to make it stay alive even if it goes out of scope */
        self->have_floating_ref = TRUE;
        Py_INCREF((PyObject *) self);
    }
    return (PyObject *) self;
}

static PyObject *
pygobject_weak_ref_unref(PyGObjectWeakRef *self, PyObject *args)
{
    if (!self->obj) {
        PyErr_SetString(PyExc_ValueError, "weak ref already unreffed");
        return NULL;
    }
    g_object_weak_unref(self->obj, (GWeakNotify) pygobject_weak_ref_notify, self);
    self->obj = NULL;
    if (self->have_floating_ref) {
        self->have_floating_ref = FALSE;
        Py_DECREF(self);
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef pygobject_weak_ref_methods[] = {
    { "unref", (PyCFunction)pygobject_weak_ref_unref, METH_NOARGS},
    { NULL, NULL, 0}
};

static PyObject *
pygobject_weak_ref_call(PyGObjectWeakRef *self, PyObject *args, PyObject *kw)
{
    static char *argnames[] = {NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kw, ":__call__", argnames))
        return NULL;

    if (self->obj)
        return pygobject_new_full(self->obj, FALSE, NULL);
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

static gpointer
pyobject_copy(gpointer boxed)
{
    PyObject *object = boxed;

    Py_INCREF(object);
    return object;
}

static void
pyobject_free(gpointer boxed)
{
    PyObject *object = boxed;
    PyGILState_STATE state;

    state = pyglib_gil_state_ensure();
    Py_DECREF(object);
    pyglib_gil_state_release(state);
}
#endif

void
hbgobject_object_register_types(void)
{
#if 0
    PyObject *o, *descr;
#endif

    hbgobject_class_key = g_quark_from_static_string("PyGObject::class");
#if 0
    pygobject_class_init_key = g_quark_from_static_string("PyGObject::class-init");
#endif
    hbgobject_wrapper_key = g_quark_from_static_string("HbGObject::wrapper");
#if 0
    pygobject_has_updated_constructor_key =
        g_quark_from_static_string("PyGObject::has-updated-constructor");
#endif
    hbgobject_instance_data_key = g_quark_from_static_string("PyGObject::instance-data");
    hbgobject_ref_sunk_key = g_quark_from_static_string("HbGObject::ref-sunk");
    hbgobject_private_flags_key = g_quark_from_static_string("HbGObject::private-flags");

    /* GObject */
    if (!HB_TYPE_ITEM)
	HB_TYPE_ITEM = g_boxed_type_register_static("HB_ITEM",
						    hb_itemNew,
						    (GBoxedFreeFunc)hb_itemRelease);
#if 0
    PyGObject_Type.tp_dealloc = (destructor)pygobject_dealloc;
    PyGObject_Type.tp_richcompare = pygobject_richcompare;
    PyGObject_Type.tp_repr = (reprfunc)pygobject_repr;
    PyGObject_Type.tp_hash = (hashfunc)pygobject_hash;
    PyGObject_Type.tp_setattro = (setattrofunc)pygobject_setattro;
    PyGObject_Type.tp_flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
			       Py_TPFLAGS_HAVE_GC);
    PyGObject_Type.tp_traverse = (traverseproc)pygobject_traverse;
    PyGObject_Type.tp_weaklistoffset = offsetof(PyGObject, weakreflist);
    PyGObject_Type.tp_methods = pygobject_methods;
    PyGObject_Type.tp_getset = pygobject_getsets;
    PyGObject_Type.tp_dictoffset = offsetof(PyGObject, inst_dict);
    PyGObject_Type.tp_init = (initproc)pygobject_init;
    PyGObject_Type.tp_free = (freefunc)pygobject_free;
    PyGObject_Type.tp_alloc = PyType_GenericAlloc;
    PyGObject_Type.tp_new = PyType_GenericNew;
#endif
    HbGObject_Type = hbgobject_register_class("GObject", G_TYPE_OBJECT, NULL);
    hbgi_hb_clsAddMsg(HbGObject_Type, "__del__", HB_OO_MSG_DESTRUCTOR, 0, HB_FUNCNAME(hbgobject_clear), NULL);
    hbgi_hb_clsAddMsg(HbGObject_Type, "get_property", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_get_property), NULL);
    hbgi_hb_clsAddMsg(HbGObject_Type, "get_properties", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_get_properties), NULL);
    //hbgi_hb_clsAddMsg(HbGObject_Type, "set_property", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_set_property), NULL);
    //hbgi_hb_clsAddMsg(HbGObject_Type, "set_properties", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_set_properties), NULL);
    hbgi_hb_clsAddMsg(HbGObject_Type, "freeze_notify", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_freeze_notify), NULL);
    hbgi_hb_clsAddMsg(HbGObject_Type, "notify", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_notify), NULL);
    hbgi_hb_clsAddMsg(HbGObject_Type, "thaw_notify", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_thaw_notify), NULL);
    hbgi_hb_clsAddMsg(HbGObject_Type, "get_data", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_get_data), NULL);
    hbgi_hb_clsAddMsg(HbGObject_Type, "set_data", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_set_data), NULL);
    hbgi_hb_clsAddMsg(HbGObject_Type, "connect", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_connect), NULL);
    hbgi_hb_clsAddMsg(HbGObject_Type, "connect_after", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_connect_after), NULL);
    hbgi_hb_clsAddMsg(HbGObject_Type, "connect_object", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_connect_object), NULL);
    hbgi_hb_clsAddMsg(HbGObject_Type, "connect_object_after", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_connect_object_after), NULL);
    hbgi_hb_clsAddMsg(HbGObject_Type, "disconnect", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_disconnect), NULL);
    //hbgi_hb_clsAddMsg(HbGObject_Type, "disconnect_by_func", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_disconnect_by_func), NULL);
    hbgi_hb_clsAddMsg(HbGObject_Type, "handler_disconnect", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_disconnect), NULL);
    hbgi_hb_clsAddMsg(HbGObject_Type, "handler_is_connected", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_handler_is_connected), NULL);
    hbgi_hb_clsAddMsg(HbGObject_Type, "handler_block", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_handler_block), NULL);
    hbgi_hb_clsAddMsg(HbGObject_Type, "handler_unblock", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_handler_unblock), NULL);
    //hbgi_hb_clsAddMsg(HbGObject_Type, "handler_block_by_func", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_handler_block_by_func), NULL);
    //hbgi_hb_clsAddMsg(HbGObject_Type, "handler_unblock_by_func", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_handler_unblock_by_func), NULL);
    //hbgi_hb_clsAddMsg(HbGObject_Type, "emit", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_emit), NULL);
    hbgi_hb_clsAddMsg(HbGObject_Type, "stop_emission", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_stop_emission), NULL);
    hbgi_hb_clsAddMsg(HbGObject_Type, "emit_stop_by_name", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_stop_emission), NULL);
    //hbgi_hb_clsAddMsg(HbGObject_Type, "chain", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_chain_from_overridden), NULL);
    //hbgi_hb_clsAddMsg(HbGObject_Type, "weak_ref", HB_OO_MSG_METHOD, 0, HB_FUNCNAME(hbgobject_weak_ref), NULL);
    /*PyDict_SetItemString(PyGObject_Type.tp_dict, "__gdoc__",
			 pyg_object_descr_doc_get());*/
    //pyg_set_object_has_new_constructor(G_TYPE_OBJECT);

#if 0
    /* GProps */
    PyGProps_Type.tp_dealloc = (destructor)PyGProps_dealloc;
    PyGProps_Type.tp_as_sequence = (PySequenceMethods*)&_PyGProps_as_sequence;
    PyGProps_Type.tp_getattro = (getattrofunc)PyGProps_getattro;
    PyGProps_Type.tp_setattro = (setattrofunc)PyGProps_setattro;
    PyGProps_Type.tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC;
    PyGProps_Type.tp_doc = "The properties of the GObject accessible as "
	"Python attributes.";
    PyGProps_Type.tp_traverse = (traverseproc)pygobject_props_traverse;
    PyGProps_Type.tp_iter = (getiterfunc)pygobject_props_get_iter;
    if (PyType_Ready(&PyGProps_Type) < 0)
        return;

    /* GPropsDescr */
    PyGPropsDescr_Type.tp_flags = Py_TPFLAGS_DEFAULT;
    PyGPropsDescr_Type.tp_descr_get = pyg_props_descr_descr_get;
    if (PyType_Ready(&PyGPropsDescr_Type) < 0)
        return;
    descr = PyObject_New(PyObject, &PyGPropsDescr_Type);
    PyDict_SetItemString(PyGObject_Type.tp_dict, "props", descr);
    PyDict_SetItemString(PyGObject_Type.tp_dict, "__module__",
                        o=PYGLIB_PyUnicode_FromString("gobject._gobject"));
    Py_DECREF(o);

    /* GPropsIter */
    PyGPropsIter_Type.tp_dealloc = (destructor)pyg_props_iter_dealloc;
    PyGPropsIter_Type.tp_flags = Py_TPFLAGS_DEFAULT;
    PyGPropsIter_Type.tp_doc = "GObject properties iterator";
    PyGPropsIter_Type.tp_iternext = (iternextfunc)pygobject_props_iter_next;
    if (PyType_Ready(&PyGPropsIter_Type) < 0)
        return;

    PyGObjectWeakRef_Type.tp_dealloc = (destructor)pygobject_weak_ref_dealloc;
    PyGObjectWeakRef_Type.tp_call = (ternaryfunc)pygobject_weak_ref_call;
    PyGObjectWeakRef_Type.tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC;
    PyGObjectWeakRef_Type.tp_doc = "A GObject weak reference";
    PyGObjectWeakRef_Type.tp_traverse = (traverseproc)pygobject_weak_ref_traverse;
    PyGObjectWeakRef_Type.tp_clear = (inquiry)pygobject_weak_ref_clear;
    PyGObjectWeakRef_Type.tp_methods = pygobject_weak_ref_methods;
    if (PyType_Ready(&PyGObjectWeakRef_Type) < 0)
        return;
    PyDict_SetItemString(d, "GObjectWeakRef", (PyObject *) &PyGObjectWeakRef_Type);
#endif
}

static void
hbgobject_init(void *dummy)
{
   HB_SYMBOL_UNUSED(dummy);
   hbgobject_pointer_register_types();
   hbgobject_object_register_types();
}

HB_CALL_ON_STARTUP_BEGIN( _hbgobject_init_ )
   hb_vmAtInit( hbgobject_init, NULL );
HB_CALL_ON_STARTUP_END( _hbgobject_init_ )

#if defined( HB_PRAGMA_STARTUP )
   #pragma startup _hbgobject_init_
#elif defined( HB_DATASEG_STARTUP )
   #define HB_DATASEG_BODY  HB_DATASEG_FUNC( _hbgobject_init_ )
   #include "hbiniseg.h"
#endif
