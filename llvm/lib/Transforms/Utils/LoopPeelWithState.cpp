#include "llvm/Transforms/Utils/LoopPeelWithState.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Transforms/Utils/LoopPeel.h"
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

  return PreservedAnalyses::none();
}

bool LoopPeelWithStatePass::hasStateVariables(Loop &L) {
  BasicBlock *Header = L.getHeader();

  BasicBlock *Latch = L.getLoopLatch();

  for (PHINode &Phi : Header->phis()) {
    if (Phi.getBasicBlockIndex(Latch) < 0)
      continue;

    Value *LatchValue = Phi.getIncomingValueForBlock(Latch);
    if (isDerivedFromIndVar(LatchValue, L))
      return true;
  }

  return false;
}

bool LoopPeelWithStatePass::isDerivedFromIndVar(Value *DerivedValue, Loop &L) {
  return DerivedValue == L.getCanonicalInductionVariable();
}
