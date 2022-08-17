#include <stdio.h>
#include "mlir-c/IR.h"
#include "mlir-c/Debug.h"
#include "mlir-c/Dialect/LLVM.h"
#include "mlir-c/ExecutionEngine.h"
#include "mlir-c/BuiltinAttributes.h"

typedef void (*func)(long long a, long long b);

void print()
{
    printf("print from code\n");
}

int main()
{
    mlirEnableGlobalDebug(true);

    MlirContext ctx = mlirContextCreate();
    MlirDialectHandle llvm = mlirGetDialectHandle__llvm__();
    mlirDialectHandleRegisterDialect(llvm, ctx);
    mlirDialectHandleLoadDialect(llvm, ctx);
    mlirRegisterAllLLVMTranslations(ctx);

    MlirLocation location = mlirLocationUnknownGet(ctx);
    MlirModule module = mlirModuleCreateEmpty(location);
    MlirBlock moduleBody = mlirModuleGetBody(module);
    
    MlirIdentifier type_id = mlirIdentifierGet(ctx, mlirStringRefCreateFromCString("type"));
    MlirIdentifier syn_name_id = mlirIdentifierGet(ctx, mlirStringRefCreateFromCString("sym_name"));

    MlirType i64Type = mlirTypeParseGet(ctx, mlirStringRefCreate("i64", 3));

    {
        MlirRegion region = mlirRegionCreate();
        MlirAttribute funcTypeAttr = mlirAttributeParseGet(ctx, mlirStringRefCreateFromCString("!llvm.func<void ()>"));
        MlirAttribute funcNameAttr = mlirAttributeParseGet(ctx, mlirStringRefCreateFromCString("\"printf\""));
        MlirNamedAttribute funcAttrs[] = {
            mlirNamedAttributeGet(type_id, funcTypeAttr),
            mlirNamedAttributeGet(syn_name_id, funcNameAttr)};
        MlirOperationState funcState = mlirOperationStateGet(
            mlirStringRefCreateFromCString("llvm.func"), location);
        mlirOperationStateAddAttributes(&funcState, 2, funcAttrs);
        mlirOperationStateAddOwnedRegions(&funcState, 1, &region);
        MlirOperation func = mlirOperationCreate(&funcState);
        mlirBlockInsertOwnedOperation(moduleBody, 0, func);
    }

    {
        MlirType funcBodyArgTypes[] = {i64Type, i64Type};
        MlirLocation funcBodyArgLocs[] = {location, location};
        MlirRegion funcBodyRegion = mlirRegionCreate();
        MlirBlock funcBody = mlirBlockCreate(2, funcBodyArgTypes, funcBodyArgLocs);
        mlirRegionAppendOwnedBlock(funcBodyRegion, funcBody);

        MlirAttribute funcTypeAttr = mlirAttributeParseGet(ctx, mlirStringRefCreateFromCString("!llvm.func<void (i64, i64)>"));
        MlirAttribute funcNameAttr = mlirAttributeParseGet(ctx, mlirStringRefCreateFromCString("\"add\""));
        MlirAttribute unitAttr = mlirUnitAttrGet(ctx);
        MlirNamedAttribute funcAttrs[] = {
            mlirNamedAttributeGet(type_id, funcTypeAttr),
            mlirNamedAttributeGet(syn_name_id, funcNameAttr)};
        MlirOperationState funcState = mlirOperationStateGet(
            mlirStringRefCreateFromCString("llvm.func"), location);
        mlirOperationStateAddAttributes(&funcState, 2, funcAttrs);
        mlirOperationStateAddOwnedRegions(&funcState, 1, &funcBodyRegion);
        MlirOperation func = mlirOperationCreate(&funcState);
        mlirBlockInsertOwnedOperation(moduleBody, 1, func);

        MlirOperationState callState = mlirOperationStateGet(
            mlirStringRefCreateFromCString("llvm.call"), location);
        MlirNamedAttribute callee = mlirNamedAttributeGet(
                mlirIdentifierGet(ctx, mlirStringRefCreateFromCString("callee")),
                mlirFlatSymbolRefAttrGet(ctx, mlirStringRefCreateFromCString("printf")));
        mlirOperationStateAddAttributes(&callState, 1, &callee);
        MlirOperation call = mlirOperationCreate(&callState);
        mlirBlockInsertOwnedOperation(funcBody, 0, call);

        MlirOperationState retState = mlirOperationStateGet(
            mlirStringRefCreateFromCString("llvm.return"), location);
        MlirOperation ret = mlirOperationCreate(&retState);
        mlirBlockInsertOwnedOperationAfter(funcBody, call, ret);
    }

    MlirOperation moduleOp = mlirModuleGetOperation(module);
    mlirOperationVerify(moduleOp);
    mlirOperationDump(moduleOp);

    MlirExecutionEngine engine = mlirExecutionEngineCreate(module, 0, 0, NULL);
    mlirExecutionEngineRegisterSymbol(engine, mlirStringRefCreateFromCString("printf"), print);

    func f = mlirExecutionEngineLookupPacked(engine, mlirStringRefCreateFromCString("add"));
    f(1, 2);

    return 0;
}