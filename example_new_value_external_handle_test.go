package v8go_test

import (
	"fmt"
	"runtime/cgo"
	"strings"

	v8 "github.com/tommie/v8go"
)

func Example_wrappingNativeGoObject() {
	// This example shows how to create a JavaScript class that is a wrapper for
	// a native Go type.
	//
	// This example uses a FunctionTemplate to expose a class that can be
	// constructed from JavaScript.

	iso := v8.NewIsolate()
	defer iso.Dispose()

	// The JavaScript object need to have some kind of "handle" referring to the
	// original object. This example uses cgo handles which exist for this very
	// purpose.
	//
	// We keep a list of created handles in order to clean up, as active handle
	// will prevent the Go value from being gargabe collected.
	var handles []cgo.Handle
	defer func() {
		for _, h := range handles {
			h.Delete()
		}
	}()

	constructor := v8.NewFunctionTemplateWithError(iso,
		func(info *v8.FunctionCallbackInfo) (*v8.Value, error) {
			handle := cgo.NewHandle(&strings.Builder{})
			handles = append(handles, handle)
			info.This().SetInternalField(0, v8.NewValueExternalHandle(iso, handle))
			return nil, nil
		})

	// A simple helper to retrieve the internal value. The checks for internal
	// field count is necessary to protect against this type of misuse:
	//
	// 	const notABuilder = { __proto__: Builder }
	// 	notABuilder.writeString("value")
	//
	// The idiomatic result should be a JavaScript TypeError thrown.
	getInstance := func(info *v8.FunctionCallbackInfo) (*strings.Builder, error) {
		if info.This().InternalFieldCount() > 0 {
			if handle := info.This().GetInternalField(0).ExternalHandle(); handle != 0 {
				if builder, ok := handle.Value().(*strings.Builder); ok {
					return builder, nil
				}
			}
		}
		return nil, v8.NewTypeError(iso, "Object is not an instance of the Builder interface")
	}

	// You must call SetInternalFieldCount on the InstanceTemplate before
	// setting an internal field on an actual instance.
	constructor.InstanceTemplate().SetInternalFieldCount(1)

	// Methods are added to the PrototypeTemplate
	constructor.PrototypeTemplate().Set("writeString", v8.NewFunctionTemplateWithError(iso,
		func(info *v8.FunctionCallbackInfo) (*v8.Value, error) {
			builder, err := getInstance(info)
			if err != nil {
				return nil, err
			}
			if len(info.Args()) == 0 {
				return nil, v8.NewTypeError(iso, "Missing argument, s")
			}
			builder.WriteString(info.Args()[0].String())
			return nil, nil
		}))
	constructor.PrototypeTemplate().Set("toString", v8.NewFunctionTemplateWithError(iso,
		func(info *v8.FunctionCallbackInfo) (*v8.Value, error) {
			builder, err := getInstance(info)
			if err != nil {
				return nil, err
			}
			return v8.NewValue(iso, builder.String())
		}))

	// Create a template for global scope, and add the builder to it
	global := v8.NewObjectTemplate(iso)
	global.Set("StringBuilder", constructor)

	ctx := v8.NewContext(iso, global)
	defer ctx.Close()

	val, _ := ctx.RunScript(`
		const b = new StringBuilder()
		b.writeString("Hello ")
		b.writeString("from ")
		b.writeString("JavaScript!")
		b.toString()
	`, "")
	fmt.Println("First batch")
	fmt.Println(val.String())

	// Output:
	// First batch
	// Hello from JavaScript!
}
