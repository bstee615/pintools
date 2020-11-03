package main

import (
	"fmt"
	"reflect"

	"github.com/llir/llvm/asm"
	"github.com/llir/llvm/ir"
	"github.com/llir/llvm/ir/metadata"
)

func main() {
	// Parse LLVM IR assembly file.
	m, err := asm.ParseFile("../test.ll")
	if err != nil {
		panic(err)
	}
	faultLocations := []FaultLocation{
		{
			filename:   "test.c",
			lineNumber: 17,
		},
	}
	f := faults(m, faultLocations)
	for loc, vs := range f {
		fmt.Printf("%v: [ ", loc)
		for _, v := range vs {
			fmt.Printf("%s ", v)
		}
		fmt.Printf("]\n")
	}
}

type FaultLocation struct {
	lineNumber int64
	filename   string
}

// Filter vars to the variables visible by inst_scope
func Filter_to_scope(inst_scope metadata.Field, vars []metadata.DILocalVariable) []metadata.DILocalVariable {
	in_scope := make([]metadata.DILocalVariable, 0)
	for _, v := range vars {
		p := inst_scope
		at_root := false
		for !at_root {
			if p == v.Scope {
				in_scope = append(in_scope, v)
			}

			switch pt := p.(type) {
			case *metadata.DILocation:
				p = pt.Scope
			case *metadata.DILexicalBlock:
				p = pt.Scope
			case *metadata.DISubprogram:
				p = pt.Scope
			case *metadata.DIFile:
				// Done
				at_root = true
			}
		}
	}
	return in_scope
}

// Cool snippet from https://mrwaggel.be/post/golang-reflect-if-initialized-struct-has-member-method-or-fields/
func ReflectStructField(Iface interface{}, FieldName string) (ir.Metadata, error) {
	ValueIface := reflect.ValueOf(Iface)

	// Check if the passed interface is a pointer
	if ValueIface.Type().Kind() != reflect.Ptr {
		// Create a new type of Iface's Type, so we have a pointer to work with
		ValueIface = reflect.New(reflect.TypeOf(Iface))
	}

	// 'dereference' with Elem() and get the field by name
	Field := ValueIface.Elem().FieldByName(FieldName)
	if !Field.IsValid() {
		return nil, fmt.Errorf("Interface `%s` does not have the field `%s`", ValueIface.Type(), FieldName)
	}
	return Field.Interface().(ir.Metadata), nil
}

func faults(mod *ir.Module, faultLocations []FaultLocation) map[FaultLocation][]string {
	ret := make(map[FaultLocation][]string, 0)
	for _, loc := range faultLocations {
		if loc.filename == mod.SourceFilename {
			ret[loc] = make([]string, 0)
		}
	}
	for _, global := range mod.Globals {
		switch n := global.Metadata[0].Node.(type) {
		case *metadata.DIGlobalVariableExpression:
			for loc, _ := range ret {
				ret[loc] = append(ret[loc], n.Var.Name)
				// // case *metadata.DIGlobalVariable:
				// 	vars = append(vars, n)
			}
		}
	}

	for _, f := range mod.Funcs {
		vars := make([]metadata.DILocalVariable, 0)
		for _, block := range f.Blocks {
			for _, inst := range block.Insts {
				switch inst := inst.(type) {
				case *ir.InstCall:
					// LLVM calls a dbg function whenever a local variable or parameter is declared.
					if inst.Callee.(*ir.Func).GlobalIdent.GlobalName == "llvm.dbg.declare" {
						a := inst.Args[1].(*metadata.Value).Value.(*metadata.DILocalVariable)
						vars = append(vars, *a)
					}
				default:
					if attachments, err := ReflectStructField(inst, "Metadata"); err == nil {
						for loc, _ := range ret {
							if len(attachments) > 0 {
								inst_loc := attachments[0].Node.(*metadata.DILocation)
								if inst_loc.Line == loc.lineNumber {
									// This is the line of our fault location. Get all variables in scope.
									in_scope := Filter_to_scope(inst_loc.Scope, vars)
									for _, v := range in_scope {
										add := true
										for _, e := range ret[loc] {
											if e == v.Name {
												add = false
											}
										}
										if add {
											ret[loc] = append(ret[loc], v.Name)
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return ret
}
