package main

import (
	"fmt"

	"github.com/llir/llvm/asm"
	"github.com/llir/llvm/ir"
	"github.com/llir/llvm/ir/metadata"
)

func main() {
	// Parse LLVM IR assembly file.
	m, err := asm.ParseFile("test.ll")
	if err != nil {
		panic(err)
	}
	faultLocations := []string{"test.c:7"}
	faults(m, faultLocations)
}

func faults(mod *ir.Module, faultLocations []string) {
	fmt.Printf("SourceFilename %s\n", mod.SourceFilename)
	// For each function of the module.
	for _, f := range mod.Funcs {
		// For each basic block of the function.
		for _, block := range f.Blocks {
			// For each non-branching instruction of the basic block.
			for _, inst := range block.Insts {
				// Type switch on instruction to find call instructions.
				switch inst := inst.(type) {
				case *ir.InstStore:
					// Add edges from caller to callee.
					for _, m := range inst.Metadata {
						switch n := m.Node.(type) {
						case *metadata.DILocation:
							fmt.Printf("%d\n", n.Line)
							location := fmt.Sprintf("%s:%d", mod.SourceFilename, n.Line)
							for _, l := range faultLocations {
								if location == l {
									fmt.Printf("Fault %s\n", location)
								}
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
}