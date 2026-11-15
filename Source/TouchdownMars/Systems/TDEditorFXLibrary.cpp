// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "Systems/TDEditorFXLibrary.h"

#include "NiagaraEmitterHandle.h"
#include "NiagaraSystem.h"

UObject* UTDEditorFXLibrary::GetLightweightEmitter(UNiagaraSystem* System, int32 Index)
{
	if (!System || !System->GetEmitterHandles().IsValidIndex(Index))
	{
		return nullptr;
	}
	// UNiagaraStatelessEmitter is an internal Niagara type: hand it out as a plain UObject (single inheritance).
	return reinterpret_cast<UObject*>(System->GetEmitterHandles()[Index].GetStatelessEmitter());
}

int32 UTDEditorFXLibrary::GetEmitterCount(UNiagaraSystem* System)
{
	return System ? System->GetEmitterHandles().Num() : 0;
}

void UTDEditorFXLibrary::RefreshSystem(UNiagaraSystem* System)
{
#if WITH_EDITOR
	if (!System)
	{
		return;
	}
	for (int32 Index = 0; Index < System->GetEmitterHandles().Num(); ++Index)
	{
		if (UObject* Emitter = GetLightweightEmitter(System, Index))
		{
			Emitter->Modify();
			Emitter->PostEditChange();
		}
	}
	System->Modify();
	System->RequestCompile(true);
	System->WaitForCompilationComplete(true, false);
	System->PostEditChange();
	System->MarkPackageDirty();
#endif
}

void UTDEditorFXLibrary::NotifyEdited(UObject* Object)
{
#if WITH_EDITOR
	if (Object)
	{
		Object->Modify();
		Object->PostEditChange();
		Object->MarkPackageDirty();
	}
#endif
}

FString UTDEditorFXLibrary::DumpProperties(UObject* Object)
{
	FString Out;
	if (!Object)
	{
		return Out;
	}
	for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
	{
		FString Text;
		It->ExportTextItem_Direct(Text, It->ContainerPtrToValuePtr<void>(Object), nullptr, Object, PPF_None);
		Out += FString::Printf(TEXT("%s [%s] = %s"), *It->GetName(), *It->GetCPPType(), *Text) + LINE_TERMINATOR;
	}
	return Out;
}

bool UTDEditorFXLibrary::SetPropertyText(UObject* Object, const FString& Name, const FString& Value)
{
	if (!Object)
	{
		return false;
	}
	FProperty* Property = Object->GetClass()->FindPropertyByName(FName(*Name));
	if (!Property)
	{
		return false;
	}
#if WITH_EDITOR
	Object->Modify();
	Object->PreEditChange(Property);
#endif
	const TCHAR* Result = Property->ImportText_Direct(*Value, Property->ContainerPtrToValuePtr<void>(Object), Object, PPF_None);
#if WITH_EDITOR
	FPropertyChangedEvent Event(Property, EPropertyChangeType::ValueSet);
	Object->PostEditChangeProperty(Event);
#endif
	return Result != nullptr;
}

UObject* UTDEditorFXLibrary::GetArrayObject(UObject* Object, const FString& Name, int32 Index)
{
	if (!Object)
	{
		return nullptr;
	}
	const FArrayProperty* Array = CastField<FArrayProperty>(Object->GetClass()->FindPropertyByName(FName(*Name)));
	const FObjectPropertyBase* Inner = Array ? CastField<FObjectPropertyBase>(Array->Inner) : nullptr;
	if (!Inner)
	{
		return nullptr;
	}
	FScriptArrayHelper Helper(Array, Array->ContainerPtrToValuePtr<void>(Object));
	return Helper.IsValidIndex(Index) ? Inner->GetObjectPropertyValue(Helper.GetRawPtr(Index)) : nullptr;
}
