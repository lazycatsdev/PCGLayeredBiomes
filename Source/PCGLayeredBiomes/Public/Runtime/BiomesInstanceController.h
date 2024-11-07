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
#include "BiomesSpawnManager.h"
#include "InstancedStaticMeshDelegates.h"
#include "PCGSpawnStructures.h"
#include "UObject/Object.h"
#include "Engine/StaticMesh.h"
#include "Subsystems/WorldSubsystem.h"
#include "Grid/PCGPartitionActor.h"
#include "BiomesInstanceController.generated.h"

struct FBiomesInstanceHandle;

USTRUCT(BlueprintType, Category=Biomes)
struct PCGLAYEREDBIOMES_API FBiomesInstanceData
{
	GENERATED_BODY()

	explicit operator bool () const { return Id != INDEX_NONE && ComponentName != NAME_None; }
	bool operator==(const FBiomesInstanceHandle& InstanceId) const;

public:
	UPROPERTY()
	int32 Id = INDEX_NONE;
	UPROPERTY()
	FTransform Transform;
	UPROPERTY()
	FName ComponentName = NAME_None;

	UPROPERTY()
	int32 Custom_SetIndex = INDEX_NONE;
	UPROPERTY()
	int32 Custom_ActorIndex = INDEX_NONE;
};



USTRUCT(BlueprintType)
struct PCGLAYEREDBIOMES_API FBiomesPartition
{
	GENERATED_BODY()

	bool operator==(const FBiomesPartition& Other) const
	{
		return (GridCoord == Other.GridCoord) && (GridSize == Other.GridSize);
	}

	friend uint32 GetTypeHash(const FBiomesPartition& Key)
	{
		return HashCombine(GetTypeHash(Key.GridCoord), GetTypeHash(Key.GridSize));
	}
	
	UPROPERTY()
	FIntVector GridCoord = FIntVector::ZeroValue;
	UPROPERTY()
	uint32 GridSize = 0;
};


USTRUCT(BlueprintType)
struct PCGLAYEREDBIOMES_API FBiomesPersistentPartitionedInstances
{
	GENERATED_BODY()

	UPROPERTY()
	FBiomesPartition Partition;

	UPROPERTY()
	TArray<FBiomesInstanceData> Instances;
};

USTRUCT(BlueprintType)
struct PCGLAYEREDBIOMES_API FBiomesPersistentMainInstances
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid Guid;

	UPROPERTY()
	TArray<FBiomesInstanceData> Instances;
};

USTRUCT(BlueprintType)
struct PCGLAYEREDBIOMES_API FBiomesPersistentInstancesData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FGuid> Mains;
	UPROPERTY()
	TArray<FBiomesPartition> Partitions;
	
	UPROPERTY()
	TArray<FBiomesPersistentMainInstances> MainInstances;
	UPROPERTY()
	TArray<FBiomesPersistentPartitionedInstances> PartitionedInstances;
};

USTRUCT()
struct FBiomesISMList
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Components;
};

/**
 * 
 */
UCLASS()
class PCGLAYEREDBIOMES_API UBiomesInstanceController : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UBiomesInstanceController();

	using FBiomesInstances = TArray<FBiomesInstanceData>;
	using FTrackedComponents = TMap<TWeakObjectPtr<const UInstancedStaticMeshComponent>, TArray<int32>>;
	
	static UBiomesInstanceController* GetInstance(const UObject* WorldContext);
	
	FBiomesInstanceHandle RemoveInstance(UInstancedStaticMeshComponent* Component, int32 InstanceId);
	bool RestoreInstance(const FBiomesInstanceHandle& InstanceHandle);

	bool GetInstanceTransform(const FBiomesInstanceHandle& InstanceHandle, FTransform& InstanceTransform);
	UBiomesInstanceUserData* GetUserData(const FBiomesInstanceHandle& InstanceHandle);

	/**
	 * Return structure which contains all information about removed instances in the world.
	 * Can be stored anywhere and restored later with SetPersistentData
	 */
	UFUNCTION(BlueprintCallable, Category=Biomes)
	FBiomesPersistentInstancesData GetPersistentData() const;
	
	/**
	 * Restore information about all removed instances in the world. 
	 * @param Data Which was returned by GetPersistentData.
	 */
	UFUNCTION(BlueprintCallable, Category=Biomes)
	void SetPersistentData(const FBiomesPersistentInstancesData& Data);

	void OnPartitionLoaded(APCGPartitionActor* PartitionActor);
	void OnPartitionUnloaded(APCGPartitionActor* PartitionActor);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void RegisterManager(UBiomesSpawnManager* Manager);
	void UnRegisterManager(UBiomesSpawnManager* Manager);

protected:
	struct FResult
	{
		FBiomesInstances* Instances = nullptr;
		int32 Index = INDEX_NONE;
		AActor* Actor = nullptr;

		const FBiomesInstanceData& GetData() const { return (*Instances)[Index]; }
	};
	
	bool FindDataByHandle(const FBiomesInstanceHandle& Handle, FResult& Result);

	FBiomesInstanceHandle RemoveInstanceImpl(UInstancedStaticMeshComponent* Component, int32 InstanceId);
	bool RestoreInstanceImpl(const FName& ComponentName, const FBiomesInstanceData& Data, AActor* Actor);

	static void EnsureInstancesUnique(const FTransform& Transform, const int32 OriginalIndex, const FName ComponentName, const TArray<FBiomesInstanceData>& Instances);

	UPCGComponent* FindPcgComponent(const FGuid& Guid);
	
	const TArray<TObjectPtr<UInstancedStaticMeshComponent>>& CachePartition(AActor* Actor);
	UInstancedStaticMeshComponent* FindISM(AActor* Actor, const FName& ComponentName);
	static UInstancedStaticMeshComponent* FindISM(const FName& ComponentName, const TArray<TObjectPtr<UInstancedStaticMeshComponent>>* ISMs);
	
	FBiomesInstances* GetInstancesFor(const APCGPartitionActor* PartitionActor);

	int16 GetMainIndex(const UPCGComponent* Component);
	int16 GetPartitionIndex(const FIntVector& ActorGridCoords, uint32 ActorGridSize);

	int32 GetOriginalIndex(UInstancedStaticMeshComponent* Component, int32 InstanceId);
	void SetOriginalIndex(const UInstancedStaticMeshComponent* Component, int32 OriginalId, int32 InstanceId);
	void Track(UInstancedStaticMeshComponent* Component);
	static void InitTrackedComponent(const UInstancedStaticMeshComponent* Component, TArray<signed int>& Mapping);
	
	bool ApplyStateToActor(AActor* Actor, const FBiomesInstances& Instances, bool ConvertToLocal);
	
	void OnInstanceIndexRelocated(UInstancedStaticMeshComponent* Component, TArrayView<const FInstancedStaticMeshDelegates::FInstanceIndexUpdateData> Data);
	static void OnInstanceIndexUpdated(UInstancedStaticMeshComponent* Component, TArrayView<const FInstancedStaticMeshDelegates::FInstanceIndexUpdateData> Data);

protected:
	TMap<FGuid, FBiomesInstances> MainInstances;
	TMap<FBiomesPartition, FBiomesInstances> PartitionedInstances;

	// Persistent array of non-partitioned PCG components.
	// Indices of it are used in handles, so they should never be changed
	TArray<FGuid> Mains;
	// Persistent array of partitions. Indices of it are used in handles, so they should never be changed
	TArray<FBiomesPartition> Partitions;

	UPROPERTY(Transient)
	TMap<TObjectPtr<AActor>, FBiomesISMList> ISMMapping;
	UPROPERTY(Transient)
	TMap<FGuid, TWeakObjectPtr<UPCGComponent>> PcgComponents;
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<UBiomesSpawnManager>> Managers;
	
	FTrackedComponents TrackedComponents;
	FDelegateHandle DelegateHandle;
};

