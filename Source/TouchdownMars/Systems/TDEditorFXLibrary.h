// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TDEditorFXLibrary.generated.h"

class UNiagaraSystem;

/**
 * Editor scripting helpers for the dust VFX (Scripts/make_dust_vfx.py). Python cannot reach a Niagara system's
 * emitters (protected), so these hand the lightweight (stateless) emitter object to Python, whose modules are plain
 * editable properties, and rebuild the system afterwards. Editor only; they do nothing in a packaged game.
 */
UCLASS()
class TOUCHDOWNMARS_API UTDEditorFXLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** The lightweight emitter of emitter handle Index (null when that emitter is not a lightweight one). */
	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Editor")
	static UObject* GetLightweightEmitter(UNiagaraSystem* System, int32 Index);

	/** Number of emitter handles in the system. */
	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Editor")
	static int32 GetEmitterCount(UNiagaraSystem* System);

	/** Marks the emitters edited, recompiles the system and waits for it. */
	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Editor")
	static void RefreshSystem(UNiagaraSystem* System);

	/** Every property of Object as "Name = exported text" lines (for properties Python cannot see). */
	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Editor")
	static FString DumpProperties(UObject* Object);

	/** Imports Value (exported-text format) into the top-level property Name. Returns false when it fails. */
	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Editor")
	static bool SetPropertyText(UObject* Object, const FString& Name, const FString& Value);

	/** Element Index of an object-array property (for example the emitter's Modules or RendererProperties). */
	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Editor")
	static UObject* GetArrayObject(UObject* Object, const FString& Name, int32 Index);

	/** Sends a post-edit-change to an object edited from Python (rebuilds a stateless emitter's data). */
	UFUNCTION(BlueprintCallable, Category = "Touchdown Mars|Editor")
	static void NotifyEdited(UObject* Object);
};
