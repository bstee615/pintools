package main

import (
	"fmt"

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
			lineNumber: 14,
		},
	}
	f := faults(m, faultLocations)
	for loc, vs := range f {
		fmt.Printf("%v: [ ", loc)
		for _, v := range vs {
			fmt.Printf("%s ", v.Name)
		}
		fmt.Printf("]\n")
	}
}

type FaultLocation struct {
	lineNumber int64
	filename   string
}

func filter_to_scope(inst_scope metadata.Field, vars []metadata.DILocalVariable) []metadata.DILocalVariable {
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

func faults(mod *ir.Module, faultLocations []FaultLocation) map[FaultLocation][]metadata.DILocalVariable {
	ret := make(map[FaultLocation][]metadata.DILocalVariable, 0)
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
				case *ir.InstAdd:
					for _, loc := range faultLocations {
						inst_loc, ok := inst.Metadata[0].Node.(*metadata.DILocation)
						if ok && inst_loc.Line == loc.lineNumber {
							// This is the line of our fault location. Get all variables in scope.
							ret[loc] = filter_to_scope(inst_loc.Scope, vars)
						}
					}
				}
			}
		}
	}

	return ret
}
