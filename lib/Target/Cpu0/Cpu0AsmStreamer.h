#include "llvm/MC/MCStreamer.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/Twine.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixupKindInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSectionCOFF.h"
#include "llvm/MC/MCSectionMachO.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/PathV2.h"
#include <cctype>

using namespace llvm;

class Cpu0AsmStreamer : public MCStreamer {
protected:
  formatted_raw_ostream &OS;
  const MCAsmInfo &MAI;
  virtual void EmitCFIStartProcImpl(MCDwarfFrameInfo &Frame) {};
  virtual void EmitCFIEndProcImpl(MCDwarfFrameInfo &Frame) { Frame.End = (MCSymbol *) 1; };
private:
  OwningPtr<MCInstPrinter> InstPrinter;
  OwningPtr<MCCodeEmitter> Emitter;
  OwningPtr<MCAsmBackend> AsmBackend;

  SmallString<128> CommentToEmit;
  raw_svector_ostream CommentStream;

  unsigned IsVerboseAsm : 1;

  enum EHSymbolFlags { EHGlobal         = 1,
                       EHWeakDefinition = 1 << 1,
                       EHPrivateExtern  = 1 << 2 };
  DenseMap<const MCSymbol*, unsigned> FlagMap;

public:
  Cpu0AsmStreamer(MCContext &Context, formatted_raw_ostream &os,
                bool isVerboseAsm, bool useLoc, bool useCFI,
                bool useDwarfDirectory,
                MCInstPrinter *printer, MCCodeEmitter *emitter,
                MCAsmBackend *asmbackend,
                bool showInst)
    : MCStreamer(SK_AsmStreamer, Context), OS(os), MAI(Context.getAsmInfo()),
      InstPrinter(printer), Emitter(emitter), AsmBackend(asmbackend),
      CommentStream(CommentToEmit), IsVerboseAsm(isVerboseAsm) {
    if (InstPrinter && IsVerboseAsm)
      InstPrinter->setCommentStream(CommentStream);
  }
  ~Cpu0AsmStreamer() {}

  formatted_raw_ostream& getOStream() { return OS; }

  inline void EmitEOL() { OS << "\n"; }

  /// isVerboseAsm - Return true if this streamer supports verbose assembly at
  /// all.
  virtual bool isVerboseAsm() const { return IsVerboseAsm; }

  /// hasRawTextSupport - We support EmitRawText.
  virtual bool hasRawTextSupport() const { return true; }

  /// AddComment - Add a comment that can be emitted to the generated .s
  /// file if applicable as a QoI issue to make the output of the compiler
  /// more readable.  This only affects the MCAsmStreamer, and only when
  /// verbose assembly output is enabled.
  virtual void AddComment(const Twine &T) {};

  /// AddEncodingComment - Add a comment showing the encoding of an instruction.
  virtual void AddEncodingComment(const MCInst &Inst) {};

  /// GetCommentOS - Return a raw_ostream that comments can be written to.
  /// Unlike AddComment, you are required to terminate comments with \n if you
  /// use this method.
  virtual raw_ostream &GetCommentOS() {
    if (!IsVerboseAsm)
      return nulls();  // Discard comments unless in verbose asm mode.
    return CommentStream;
  }

  /// AddBlankLine - Emit a blank line to a .s file to pretty it up.
  virtual void AddBlankLine() {
    EmitEOL();
  }

  /// @name MCStreamer Interface
  /// @{

  virtual void ChangeSection(const MCSection *Section,
                             const MCExpr *Subsection) {};

  virtual void InitSections() {
    InitToTextSection();
  }

  virtual void InitToTextSection() {
    SwitchSection(getContext().getObjectFileInfo()->getTextSection());
  }

  virtual void EmitLabel(MCSymbol *Symbol) {};
  virtual void EmitDebugLabel(MCSymbol *Symbol) {};

  virtual void EmitEHSymAttributes(const MCSymbol *Symbol,
                                   MCSymbol *EHSymbol) {};
  virtual void EmitAssemblerFlag(MCAssemblerFlag Flag) {};
  virtual void EmitLinkerOptions(ArrayRef<std::string> Options) {};
  virtual void EmitDataRegion(MCDataRegionType Kind) {};
  virtual void EmitThumbFunc(MCSymbol *Func) {};

  virtual void EmitAssignment(MCSymbol *Symbol, const MCExpr *Value) {};
  virtual void EmitWeakReference(MCSymbol *Alias, const MCSymbol *Symbol) {};
  virtual void EmitDwarfAdvanceLineAddr(int64_t LineDelta,
                                        const MCSymbol *LastLabel,
                                        const MCSymbol *Label,
                                        unsigned PointerSize) {};
  virtual void EmitDwarfAdvanceFrameAddr(const MCSymbol *LastLabel,
                                         const MCSymbol *Label) {};

  virtual void EmitSymbolAttribute(MCSymbol *Symbol, MCSymbolAttr Attribute) {};

  virtual void EmitSymbolDesc(MCSymbol *Symbol, unsigned DescValue) {};
  virtual void BeginCOFFSymbolDef(const MCSymbol *Symbol) {};
  virtual void EmitCOFFSymbolStorageClass(int StorageClass) {};
  virtual void EmitCOFFSymbolType(int Type) {};
  virtual void EndCOFFSymbolDef() {};
  virtual void EmitCOFFSecRel32(MCSymbol const *Symbol) {};
  virtual void EmitELFSize(MCSymbol *Symbol, const MCExpr *Value) {};
  virtual void EmitCommonSymbol(MCSymbol *Symbol, uint64_t Size,
                                unsigned ByteAlignment) {};

  /// EmitLocalCommonSymbol - Emit a local common (.lcomm) symbol.
  ///
  /// @param Symbol - The common symbol to emit.
  /// @param Size - The size of the common symbol.
  /// @param ByteAlignment - The alignment of the common symbol in bytes.
  virtual void EmitLocalCommonSymbol(MCSymbol *Symbol, uint64_t Size,
                                     unsigned ByteAlignment) {};

  virtual void EmitZerofill(const MCSection *Section, MCSymbol *Symbol = 0,
                            uint64_t Size = 0, unsigned ByteAlignment = 0) {};

  virtual void EmitTBSSSymbol (const MCSection *Section, MCSymbol *Symbol,
                               uint64_t Size, unsigned ByteAlignment = 0) {};

  virtual void EmitBytes(StringRef Data, unsigned AddrSpace) {};

  virtual void EmitValueImpl(const MCExpr *Value, unsigned Size,
                             unsigned AddrSpace) {};
  virtual void EmitIntValue(uint64_t Value, unsigned Size,
                            unsigned AddrSpace = 0) {};

  virtual void EmitULEB128Value(const MCExpr *Value) {};

  virtual void EmitSLEB128Value(const MCExpr *Value) {};

  virtual void EmitGPRel64Value(const MCExpr *Value) {};

  virtual void EmitGPRel32Value(const MCExpr *Value) {};


  virtual void EmitFill(uint64_t NumBytes, uint8_t FillValue,
                        unsigned AddrSpace) {};

  virtual void EmitValueToAlignment(unsigned ByteAlignment, int64_t Value = 0,
                                    unsigned ValueSize = 1,
                                    unsigned MaxBytesToEmit = 0) {};

  virtual void EmitCodeAlignment(unsigned ByteAlignment,
                                 unsigned MaxBytesToEmit = 0) {};

  virtual bool EmitValueToOffset(const MCExpr *Offset,
                                 unsigned char Value = 0) { return true; };

  virtual void EmitFileDirective(StringRef Filename) {};
  virtual bool EmitDwarfFileDirective(unsigned FileNo, StringRef Directory,
                                      StringRef Filename, unsigned CUID = 0) { return true; };
  virtual void EmitDwarfLocDirective(unsigned FileNo, unsigned Line,
                                     unsigned Column, unsigned Flags,
                                     unsigned Isa, unsigned Discriminator,
                                     StringRef FileName) {};

  virtual void EmitCFISections(bool EH, bool Debug) {};
  virtual void EmitCFIDefCfa(int64_t Register, int64_t Offset) {};
  virtual void EmitCFIDefCfaOffset(int64_t Offset) {};
  virtual void EmitCFIDefCfaRegister(int64_t Register) {};
  virtual void EmitCFIOffset(int64_t Register, int64_t Offset) {};
  virtual void EmitCFIPersonality(const MCSymbol *Sym, unsigned Encoding) {};
  virtual void EmitCFILsda(const MCSymbol *Sym, unsigned Encoding) {};
  virtual void EmitCFIRememberState() {};
  virtual void EmitCFIRestoreState() {};
  virtual void EmitCFISameValue(int64_t Register) {};
  virtual void EmitCFIRelOffset(int64_t Register, int64_t Offset) {};
  virtual void EmitCFIAdjustCfaOffset(int64_t Adjustment) {};
  virtual void EmitCFISignalFrame() {};
  virtual void EmitCFIUndefined(int64_t Register) {};
  virtual void EmitCFIRegister(int64_t Register1, int64_t Register2) {};

  virtual void EmitWin64EHStartProc(const MCSymbol *Symbol) {};
  virtual void EmitWin64EHEndProc() {};
  virtual void EmitWin64EHStartChained() {};
  virtual void EmitWin64EHEndChained() {};
  virtual void EmitWin64EHHandler(const MCSymbol *Sym, bool Unwind,
                                  bool Except) {};
  virtual void EmitWin64EHHandlerData() {};
  virtual void EmitWin64EHPushReg(unsigned Register) {};
  virtual void EmitWin64EHSetFrame(unsigned Register, unsigned Offset) {};
  virtual void EmitWin64EHAllocStack(unsigned Size) {};
  virtual void EmitWin64EHSaveReg(unsigned Register, unsigned Offset) {};
  virtual void EmitWin64EHSaveXMM(unsigned Register, unsigned Offset) {};
  virtual void EmitWin64EHPushFrame(bool Code) {};
  virtual void EmitWin64EHEndProlog() {};

  virtual void EmitFnStart() {};
  virtual void EmitFnEnd() {};
  virtual void EmitCantUnwind() {};
  virtual void EmitPersonality(const MCSymbol *Personality) {};
  virtual void EmitHandlerData() {};
  virtual void EmitSetFP(unsigned FpReg, unsigned SpReg, int64_t Offset = 0) {};
  virtual void EmitPad(int64_t Offset) {};
  virtual void EmitRegSave(const SmallVectorImpl<unsigned> &RegList, bool) {};

  virtual void EmitTCEntry(const MCSymbol &S) {};

  virtual void EmitInstruction(const MCInst &Inst) {
    // If we have an AsmPrinter, use that to print, otherwise print the MCInst.
    if (InstPrinter)
      InstPrinter->printInst(&Inst, OS, "");
    else
      Inst.print(OS, &MAI);
    EmitEOL();
  };

  virtual void EmitBundleAlignMode(unsigned AlignPow2) {};
  virtual void EmitBundleLock(bool AlignToEnd) {};
  virtual void EmitBundleUnlock() {};

  /// EmitRawText - If this file is backed by an assembly streamer, this dumps
  /// the specified string in the output .s file.  This capability is
  /// indicated by the hasRawTextSupport() predicate.
  virtual void EmitRawText(StringRef String) {};

  virtual void FinishImpl() {};

  /// @}

  static bool classof(const MCStreamer *S) {
    return S->getKind() == SK_AsmStreamer;
  }
};
