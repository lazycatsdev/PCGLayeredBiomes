/*
 * Copyright (c) 2024 LazyCatsDev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#pragma once

#include "CoreMinimal.h"
#include "PCGSpawnStructures.h"
#include "Components/ActorComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "BiomesSpawnManager.generated.h"


class UBiomesInstanceController;
struct FBiomeSettings;
class UPCGComponent;
class UBiomesSettings;

UCLASS(Blueprintable, ClassGroup=(Biomes), meta=(BlueprintSpawnableComponent))
class PCGLAYEREDBIOMES_API UBiomesSpawnManager : public UActorComponent
{
	GENERATED_BODY()

	friend class UBiomesInstanceController;
	
public:
	static UBiomesSpawnManager* GetManager(UPCGComponent* InComponent);
	static UBiomesSpawnManager* GetManager(const AActor* Actor);
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	const TArray<FPCGSpawnInfo>* FindSet(const FString& SetName) const;
	const FBiomeSettings* FindSettings(FName BiomeName) const;

	FPCGCrc GetBiomesCrc() const;
	FSoftObjectPath GetBiomesSoftPath() const;

	FPCGCrc GetSpawnPresetCrc() const;
	FSoftObjectPath GetSpawnPresetSoftPath() const;

	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
	
#if WITH_EDITOR
	virtual void PostLoad() override;

	UFUNCTION(CallInEditor, Category="Interaction")
	void PrepareForRuntimeInteractions() const;

	void FixAllPCGActors() const;
	void OnGenerationDone(class UPCGSubsystem* Subsystem) const;

	virtual void CheckForErrors() override;
	
#endif

	UBiomesInstanceUserData* GetExtraDataFromInstance(const UInstancedStaticMeshComponent* Component,
	                                                  const int32 InstanceId) const;
	UBiomesInstanceUserData* GetExtraData(int32 SetIndex, int32 ActorIndex) const;
	bool GetSpawnInfoFromInstance(const UInstancedStaticMeshComponent* Component,
	                                       const int32 InstanceId, FPCGSpawnInfo& Result) const;
	
public:
	struct FPackedTagsEntry
	{
		int32 SetIndex = INDEX_NONE;
		int32 ActorIndex = INDEX_NONE;
	};
	
	const FPCGSpawnInfo* FindActorInfoByMesh(const TSoftObjectPtr<UStaticMesh>& Mesh, FPackedTagsEntry& OutEntry) const;
	
private:
	static FPackedTagsEntry GetTagEntryFromInstance(const UInstancedStaticMeshComponent* Component, const int32 InstanceId);
	static void SetTagEntryFromInstance(UInstancedStaticMeshComponent* Component, const int32 InstanceId, const FPackedTagsEntry& Data);
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Biomes, AdvancedDisplay, meta=(EditCondition="false"))
	FGuid Guid;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Biomes)
	TObjectPtr<UPCGSpawnPreset> Preset;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Biomes)
	TObjectPtr<UBiomesSettings> Biomes;
	
	struct FInstancedActor
	{
		FSoftObjectPath StaticMesh;
		int32 InstanceId = INDEX_NONE;
	};
	TMap<TWeakObjectPtr<AActor>, FInstancedActor> DisabledInstances; 
};
