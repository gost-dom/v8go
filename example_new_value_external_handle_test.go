package v8go_test

import (
	"fmt"
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

	constructor := v8.NewFunctionTemplateWithError(iso,
		func(info *v8.FunctionCallbackInfo) (*v8.Value, error) {
			val, err := v8.NewValue(iso, &strings.Builder{})
			if err == nil {
				err = info.This().SetInternalField(0, val)
			}
			return nil, err
		})

	// You must call SetInternalFieldCount on the InstanceTemplate before
	// setting an internal field on an actual instance.
	constructor.InstanceTemplate().SetInternalFieldCount(1)

	// A simple helper to retrieve the internal value. It is necessary to check
	// the internal field count prior to accessing the fields, as the function
	// could be associated with the wrong prototype. For example, this would
	// call the writeString method on an object that doesn't have the internal
	// value.
	//
	// 	const notABuilder = { __proto__: Builder }
	// 	notABuilder.writeString("value")
	//
	// The idiomatic JavaScript error in this case should be a TypeError
	getInternalBuilder := func(info *v8.FunctionCallbackInfo) (*strings.Builder, error) {
		if info.This().InternalFieldCount() > 0 {
			if v, ok := info.This().GetInternalField(0).External(); ok {
				if builder, ok := v.(*strings.Builder); ok {
					return builder, nil
				}
			}
		}
		return nil, v8.NewTypeError(iso, "Object is not an instance of the Builder interface")
	}

	// Add writeString to the prototype
	constructor.PrototypeTemplate().Set("writeString", v8.NewFunctionTemplateWithError(iso,
		func(info *v8.FunctionCallbackInfo) (*v8.Value, error) {
			builder, err := getInternalBuilder(info)
			if err != nil {
				return nil, err
			}
			if len(info.Args()) == 0 {
				return nil, v8.NewTypeError(iso, "Missing argument, s")
			}
			builder.WriteString(info.Args()[0].String())
			return nil, nil
		}))

	// Add toString to the prototype
	constructor.PrototypeTemplate().Set("toString", v8.NewFunctionTemplateWithError(iso,
		func(info *v8.FunctionCallbackInfo) (*v8.Value, error) {
			builder, err := getInternalBuilder(info)
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
