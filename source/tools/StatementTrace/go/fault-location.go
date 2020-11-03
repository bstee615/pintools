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

func faults(mod *ir.Module, faultLocations []FaultLocation) map[FaultLocation][]*metadata.DILocalVariable {
	fmt.Printf("SourceFilename %s\n", mod.SourceFilename)
	locationsByFunction := make(map[*ir.Func][]FaultLocation, 0)

	for _, f := range mod.Funcs {
		locationsByFunction[f] = make([]FaultLocation, 0)
		var begin int64
		if f.Metadata != nil {
			begin = f.Metadata[0].Node.(*metadata.DISubprogram).Line
		}
		if len(f.Blocks) > 0 {
			block := f.Blocks[len(f.Blocks)-1]
			if t, ok := block.Term.(*ir.TermRet); ok {
				var end int64 = t.Metadata[0].Node.(*metadata.DILocation).Line // TODO
				fmt.Printf("%s\n", t)
				for _, loc := range faultLocations {
					if loc.filename == mod.SourceFilename && begin < loc.lineNumber && loc.lineNumber < end {
						locationsByFunction[f] = append(locationsByFunction[f], loc)
					}
				}
			}
		}
	}
	fmt.Printf("%v\n", locationsByFunction)

	ret := make(map[FaultLocation][]*metadata.DILocalVariable, 0)
	for _, f := range mod.Funcs {
		vars := make([]*metadata.DILocalVariable, 0)
		for _, block := range f.Blocks {
			for _, inst := range block.Insts {
				switch inst := inst.(type) {
				case *ir.InstCall:
					// LLVM calls a dbg function whenever a local variable or parameter is declared
					if inst.Callee.(*ir.Func).GlobalIdent.GlobalName == "llvm.dbg.declare" {
						a := inst.Args[1].(*metadata.Value).Value.(*metadata.DILocalVariable)
						vars = append(vars, a)
					}
				case *ir.InstAdd:
					for _, loc := range locationsByFunction[f] {
						inst_loc, ok := inst.Metadata[0].Node.(*metadata.DILocation)
						if ok && inst_loc.Line == loc.lineNumber {
							// This is the line of our fault location. Get all variables in scope
							in_scope := make([]*metadata.DILocalVariable, 0)
							inst_scope := inst_loc.Scope
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
							ret[loc] = in_scope
						}
					}
				}
			}
		}
	}

	return ret
}
