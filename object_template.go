// Copyright 2020 Roger Chapman and the v8go contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package v8go

// #include <stdlib.h>
// #include "object_template.h"
import "C"
import (
	"errors"
	"runtime"
	"runtime/cgo"
	"unsafe"
)

// NotIntercepted is the error returned by property handler callbacks when they
// did not intercept the property.
var NotIntercepted = errors.New("v8go: NotIntercepted")

// PropertyAttribute are the attribute flags for a property on an Object.
// Typical usage when setting an Object or TemplateObject property, and
// can also be validated when accessing a property.
type PropertyAttribute uint8

// PropertyCallbackInfo is passed when intercepting a named or indexed property.
type PropertyCallbackInfo struct {
	ctx  *Context
	args []*Value
	this *Object
	// Holder marks the object in the prototype chain that has the receiver. This
	// corresponds to `HolderV2` in the v8 API.
	holder *Object
	// True if the intercepted function should throw if an error occurs. Usually, true corresponds to ‘'use strict’`.
	interceptOnError bool
}

const (
	// None.
	None PropertyAttribute = 0
	// ReadOnly, ie. not writable.
	ReadOnly PropertyAttribute = 1 << iota
	// DontEnum, ie. not enumerable.
	DontEnum
	// DontDelete, ie. not configurable.
	DontDelete
)

// ObjectTemplate is used to create objects at runtime.
// Properties added to an ObjectTemplate are added to each object created from the ObjectTemplate.
type ObjectTemplate struct {
	*template
}

// NewObjectTemplate creates a new ObjectTemplate.
// The *ObjectTemplate can be used as a v8go.ContextOption to create a global object in a Context.
func NewObjectTemplate(iso *Isolate) *ObjectTemplate {
	if iso == nil {
		panic("nil Isolate argument not supported")
	}

	tmpl := &template{
		ptr: C.NewObjectTemplate(iso.ptr),
		iso: iso,
	}
	runtime.SetFinalizer(tmpl, (*template).finalizer)
	return &ObjectTemplate{tmpl}
}

// NewInstance creates a new Object based on the template.
func (o *ObjectTemplate) NewInstance(ctx *Context) (*Object, error) {
	if ctx == nil {
		return nil, errors.New("v8go: Context cannot be <nil>")
	}

	rtn := C.ObjectTemplateNewInstance(o.ptr, ctx.ptr)
	runtime.KeepAlive(o)
	return objectResult(ctx, rtn)
}

// SetInternalFieldCount sets the number of internal fields that instances of this
// template will have.
func (o *ObjectTemplate) SetInternalFieldCount(fieldCount uint32) {
	C.ObjectTemplateSetInternalFieldCount(o.ptr, C.int(fieldCount))
}

// SetAccessorProperty creates a named accessor property, i.e., a property that
// is implemented as a function call. Arguments get and set represents the
// getter and setter, and can both be nil.
//
// Note: The [ReadOnly] should not be used with a readonly property. If set is
// nil, the property will be readonly, and passing [None] is a sensible default.
//
// This corresponds to ObjectTemplate::SetAccessorProperty in the C++ API.
func (o *ObjectTemplate) SetAccessorProperty(
	key string,
	get *FunctionTemplate,
	set *FunctionTemplate,
	attributes PropertyAttribute,
) {
	ckey := C.CString(key)
	defer C.free(unsafe.Pointer(ckey))
	var (
		getter C.TemplatePtr
		setter C.TemplatePtr
	)
	if get != nil {
		getter = get.ptr
	}
	if set != nil {
		setter = set.ptr
	}
	C.ObjectTemplateSetAccessorProperty(o.ptr, ckey, getter, setter, C.int(attributes))
}

// InternalFieldCount returns the number of internal fields that instances of this
// template will have.
func (o *ObjectTemplate) InternalFieldCount() uint32 {
	return uint32(C.ObjectTemplateInternalFieldCount(o.ptr))
}

func (o *ObjectTemplate) apply(opts *contextOptions) {
	opts.gTmpl = o
}

// MarkAsUndetectable marks object instances of the template as undetectable. Undetectable
// objects behave like undefined, but you can access properties defined on undetectable
// objects.
//
// Note: Undetectable objects MUST have a CallAsFunctionHandler, see
// [ObjectTemplate.SetCallAsFunctionHandler]
func (o *ObjectTemplate) MarkAsUndetectable() {
	C.ObjectTemplateMarkAsUndetectable(o.ptr)
}

// SetCallAsFunctionHandler sets the callback to be used when calling instances created
// from this template. If no callback is set, instances behave like normal JavaScript
// objects that cannot be called as a function.
func (o *ObjectTemplate) SetCallAsFunctionHandler(callback FunctionCallbackWithError) {
	if callback == nil {
		panic("nil callback argument not supported")
	}
	cbref := o.iso.registerCallback(callback)
	C.ObjectTemplateSetCallAsFunctionHandler(
		o.ptr,
		C.int(cbref),
	)
}

func (o *ObjectTemplate) SetIndexedHandler(callback FunctionCallbackWithError) {
	if callback == nil {
		panic("nil FunctionCallback argument not supported")
	}

	iso := o.iso
	cbref := iso.registerCallback(callback)
	C.ObjectTemplateSetIndexHandler(o.ptr, C.int(cbref))
}

type PropertyDescriptor struct{}

type NamedPropertyCallback struct {
	ctx  *Context
	this *Object
	// Holder marks the object in the prototype chain that has the receiver. This
	// corresponds to `HolderV2` in the v8 API.
	holder *Object
}

type NamedPropertyGetter interface {
	NamedPropertyGet(property *Value, info NamedPropertyCallback) (*Value, error)
}

type NamedPropertySetter interface {
	NamedPropertySet(property *Value, value *Value, info NamedPropertyCallback) error
}

type NamedPropertyQueryer interface {
	NamedPropertyQuery(property *Value, info NamedPropertyCallback) (int, error)
}

type NamedPropertyDeleter interface {
	NamedPropertyDelete(property *Value, info NamedPropertyCallback) (success bool, err error)
}

type NamedPropertyEnumeratorer interface {
	NamedPropertyEnumerator(info NamedPropertyCallback) (names []*Value, err error)
}

type NamedPropertyDefinerer interface {
	NamedPropertyDefiner(property *Value, desc *PropertyDescriptor, info NamedPropertyCallback) error
}

type NamedPropertyDescriptorer interface {
	NamedPropertyDescriptor(property *Value, info NamedPropertyCallback) (*Value, error)
}

// SetNamedHandler allows the embedder to calculate the properties at runtime,
// for example where the embedder is exposing a map/dictionary type to
// JavaScript. The caller must provide a type implementing
// [NamedPropertyGetter], but it can optionally also support the following types
//
// - [NamedPropertySetter] to handle when a property is assigned in JavaScript
// - [NamedPropertyQueryer] to handle when property details are inspected
// - [NamedPropertyDeleter] to handle when a property is deleted
// - [NamedPropertyEnumeratorer] to return the names of the properties
// - [NamedPropertyDefiner] to handler Object.defineProperty() calls
// - [NamedPropertyDescriptor] to generate a PropertyDescriptor for a property
//
// With the exception of [NamedPropertyEnumerator] the methods accept a property
// of type [*Value]. The name can be either a string or a [*Symbol]. If the
// embedder does want to handle the callback, it must communicate this back to
// V8 by returning an [ErrNotIntercepted]. When thie is returned, the function
// must not produce any side effects.
func (t *ObjectTemplate) SetNamedHandler(handler NamedPropertyGetter) {
	if handler == nil {
		panic("nil property argument not supported")
	}
	handle := cgo.NewHandle(handler)
	cb_ref := NewValueExternalHandle(t.iso, handle)
	C.ObjectTemplateSetNamedHandler(t.ptr, cb_ref.ptr)
}

//export goNamedPropertyGetterCallback
func goNamedPropertyGetterCallback(property C.ValuePtr, info C.v8goPropertyCallbackInfo) (retVal C.ValuePtr, intercepted bool, rtnerr C.ValuePtr) {
	cbref := Value{ptr: info.cbref}
	ctx := getContext(int(info.ctx_ref))
	// Should *ALWAYS* be ok for the getter; it's required.
	cb, ok := cbref.ExternalHandle().Value().(NamedPropertyGetter)
	if !ok {
		return nil, false, nil
	}
	res, err := cb.NamedPropertyGet(&Value{ptr: property}, NamedPropertyCallback{
		ctx, &Object{&Value{ctx: ctx, ptr: info.jsThis}}, &Object{&Value{ctx: ctx, ptr: info.holder}},
	})
	intercepted = true
	if err == NotIntercepted {
		err = nil
		intercepted = false
	}
	if err != nil {
		if verr, ok := err.(ValueError); ok {
			rtnerr = verr.value().ptr
		} else {
			errv, err := NewValue(ctx.iso, err.Error())
			if err != nil {
				panic(err)
			}
			rtnerr = errv.ptr
		}
	}
	if res != nil {
		retVal = res.ptr
	}
	return
}

// func goNamedPropertySetterCallback()
// func goNamedPropertyQueryCallback()
// func goNamedPropertyDeleteCallback()
// func goNamedPropertyEnumeratorCallback()

//export goNamedPropertyEnumeratorCallback
func goNamedPropertyEnumeratorCallback(info C.v8goPropertyCallbackInfo) (retVal C.ValuePtr, intercepted bool, rtnerr C.ValuePtr) {
	cbref := Value{ptr: info.cbref}
	ctx := getContext(int(info.ctx_ref))
	// Should *ALWAYS* be ok for the getter; it's required.
	cb, ok := cbref.ExternalHandle().Value().(NamedPropertyEnumeratorer)
	if !ok {
		return nil, false, nil
	}
	res, err := cb.NamedPropertyEnumerator(NamedPropertyCallback{
		ctx, &Object{&Value{ctx: ctx, ptr: info.jsThis}}, &Object{&Value{ctx: ctx, ptr: info.holder}},
	})
	intercepted = true
	if err == NotIntercepted {
		err = nil
		intercepted = false
	}
	var retValVal *Value
	if err == nil {
		retValVal, err = toArray(ctx, res...)
	}
	if err != nil {
		if verr, ok := err.(ValueError); ok {
			rtnerr = verr.value().ptr
		} else {
			errv, err := NewValue(ctx.iso, err.Error())
			if err != nil {
				panic(err)
			}
			rtnerr = errv.ptr
		}
	}
	if res != nil {
		retVal = retValVal.ptr
	}
	return
}

func toArray(ctx *Context, values ...*Value) (*Value, error) {
	// Total hack, v8go doesn't expose Array values, so we polyfill the engine
	v, err := ctx.Global().Get("Array")
	if err != nil {
		return nil, err
	}
	obj, err := v.AsObject()
	if err != nil {
		return nil, err
	}
	of, err := obj.Get("of")
	if err != nil {
		return nil, err
	}
	fn, err := of.AsFunction()
	if err != nil {
		return nil, err
	}
	args := make([]Valuer, len(values))
	for i, v := range values {
		args[i] = v
	}
	return fn.Call(ctx.Global(), args...)
}

// func goNamedPropertyDefinerCallback()
// func goNamedPropertyDescriptorCallback()
