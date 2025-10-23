#include "llvm/Transforms/Utils/LoopPeelWithState.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Casting.h"
#include "llvm/Transforms/Utils/LoopPeel.h"
#include "llvm/Transforms/Utils/LoopPeelWithState.h"
#include "llvm/Transforms/Utils/LoopUtils.h"

using namespace llvm;

PreservedAnalyses LoopPeelWithStatePass::run(Loop &L, LoopAnalysisManager &AM,
                                             LoopStandardAnalysisResults &AR,
                                             LPMUpdater &U) {
  if (!L.isLoopSimplifyForm() || !L.getExitingBlock())
    return PreservedAnalyses::all();

  if (!hasStateVariables(L))
    return PreservedAnalyses::all();

  ValueToValueMapTy VM;
  if (!peelLoop(&L, 1, false, &AR.LI, &AR.SE, AR.DT, &AR.AC, true, VM))
    return PreservedAnalyses::all();

  Function *F = L.getHeader()->getParent();
  llvm::errs() << "Running LoopPeelWithStatePass in function: " << F->getName()
               << "\n";

  return PreservedAnalyses::none();
}

bool LoopPeelWithStatePass::hasStateVariables(Loop &L) {
  BasicBlock *Header = L.getHeader();
  BasicBlock *Latch = L.getLoopLatch();

  Value *IndVar = L.getCanonicalInductionVariable();

  for (PHINode &Phi : Header->phis()) {
    if (Phi.getBasicBlockIndex(Latch) < 0)
      continue;

    Value *LatchValue = Phi.getIncomingValueForBlock(Latch);

    if (isDerivedFromIndVar(LatchValue, L) && !(&Phi == IndVar))
      return true;
  }

  return false;
}

bool LoopPeelWithStatePass::isDerivedFromIndVar(Value *DerivedValue, Loop &L) {
  Value *IndVar = L.getCanonicalInductionVariable();
  if (!IndVar || !DerivedValue)
    return false;

  if (DerivedValue == IndVar)
    return true;

  Value *V = DerivedValue;
  while (auto *CI = dyn_cast<Instruction>(V)) {
    V = CI->getOperand(0);
    if (!V)
      return false;
    if (V == IndVar)
      return true;
  }

  return false;
}

PassPluginLibraryInfo getLoopPeelWithStatePluginInfo() {
  return {
      LLVM_PLUGIN_API_VERSION, "LoopPeelWithState", LLVM_VERSION_STRING,
      [](PassBuilder &PB) {
        PB.registerVectorizerStartEPCallback([](llvm::FunctionPassManager &PM,
                                                OptimizationLevel Level) {
          PM.addPass(createFunctionToLoopPassAdaptor(LoopPeelWithStatePass()));
        });
        PB.registerPipelineParsingCallback(
            [](StringRef Name, FunctionPassManager &FPM,
               ArrayRef<PassBuilder::PipelineElement>) {
              if (Name == "loop-peel-with-state") {
                FPM.addPass(
                    createFunctionToLoopPassAdaptor(LoopPeelWithStatePass()));
                return true;
              }
              return false;
            });
      }};
}

#ifndef LLVM_BYE_LINK_INTO_TOOLS
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getLoopPeelWithStatePluginInfo();
}
#endif
