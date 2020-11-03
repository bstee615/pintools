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
	fmt.Printf("%v\n", f)
}

type FaultLocation struct {
	lineNumber int64
	filename   string
}

type Decl struct {
	lineNumber int64
	name       string
}

func faults(mod *ir.Module, faultLocations []FaultLocation) map[FaultLocation][]Decl {
	fmt.Printf("SourceFilename %s\n", mod.SourceFilename)
	funcs := make(map[*ir.Func][]FaultLocation, 0)

	for _, f := range mod.Funcs {
		funcs[f] = make([]FaultLocation, 0)
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
					if begin < loc.lineNumber && loc.lineNumber < end {
						funcs[f] = append(funcs[f], loc)
					}
				}
			}
		}
	}

	fmt.Printf("%v\n", funcs)

	ret := make(map[FaultLocation][]Decl, 0)

	// For each function of the module.
	for _, f := range mod.Funcs {
		if f.Metadata != nil {
			fmt.Printf("func %s\n", f.Metadata[0].Node.(*metadata.DISubprogram).Name)
		}
		for _, block := range f.Blocks {
			for _, inst := range block.Insts {
				if call_inst, ok := inst.(*ir.InstCall); ok {
					if call_inst.Callee.(*ir.Func).GlobalIdent.GlobalName == "llvm.dbg.declare" {
						a := call_inst.Args[1].(*metadata.Value).Value.(*metadata.DILocalVariable)
						fmt.Printf("var %s\n", a.Name)

						// Mark all the variables declared before this fault location
						for _, loc := range funcs[f] {
							if mod.SourceFilename == loc.filename && a.Line < loc.lineNumber {
								ret[loc] = append(ret[loc], Decl{
									lineNumber: a.Line,
									name:       a.Name,
								})
							}
						}
					}
				}
			}
			// Terminator of basic block.
			switch term := block.Term.(type) {
			case *ir.TermRet:
				// do something.
				_ = term
			}
		}
	}

	return ret
}
