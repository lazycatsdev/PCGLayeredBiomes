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



#include "Runtime/BiomesInstanceController.h"

#include "BiomesLog.h"
#include "BiomesPCGUtils.h"
#include "PCGComponent.h"
#include "PCGSubsystem.h"
#include "Engine/World.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Grid/PCGPartitionActor.h"


bool FBiomesInstanceData::operator==(const FBiomesInstanceHandle& Handle) const
{
	return Id == Handle.InstanceId && ComponentName == Handle.ComponentName;
}

UBiomesInstanceController::UBiomesInstanceController()
{
}

void UBiomesInstanceController::EnsureInstancesUnique(const FTransform& Transform, const int32 OriginalIndex, const FName ComponentName, const TArray<FBiomesInstanceData>& Instances)
{
	auto* SameItem = Instances.FindByPredicate([OriginalIndex, ComponentName] (const FBiomesInstanceData& Item)
	{
		return Item.Id == OriginalIndex && Item.ComponentName == ComponentName;
	});
	ensure(SameItem == nullptr);
}

UPCGComponent* UBiomesInstanceController::FindPcgComponent(const FGuid& Guid)
{
	if (const auto* Component = PcgComponents.Find(Guid))
	{
		return Component->Get();
	}

	for (const auto& ManagerWeak: Managers)
	{
		if (const auto* Manager = ManagerWeak.Get(); Manager && Manager->Guid == Guid)
		{
			auto* Component = Manager->GetOwner()->FindComponentByClass<UPCGComponent>();
			PcgComponents.Add(Guid, Component);
			return Component;
		}
	}
	return nullptr;
}

FBiomesInstanceHandle UBiomesInstanceController::RemoveInstanceImpl(UInstancedStaticMeshComponent* Component,
                                                                    int32 InstanceId)
{
	auto* Owner = Component->GetOwner();
	check(Owner);

	auto* PcgComponent = Owner->FindComponentByClass<UPCGComponent>();
	if (!PcgComponent)
	{
		return {};
	}

	// Return a handle for original (not partitioned) component
	FTransform Transform;
	if (!Component->GetInstanceTransform(InstanceId, Transform))
	{
		return {};
	}
	const auto OriginalIndex = GetOriginalIndex(Component, InstanceId);
	if (OriginalIndex == INDEX_NONE)
	{
		ensure(false);
		return {};
	}
	
	const auto [SetIndex, ActorIndex] = UBiomesSpawnManager::GetTagEntryFromInstance(Component, InstanceId);
	const auto ComponentName = Component->GetFName();

	if (!PcgComponent->IsLocalComponent())
	{
		const auto MainIndex = GetMainIndex(PcgComponent);
		if (MainIndex == INDEX_NONE || !Mains.IsValidIndex(MainIndex))
		{
			return {};
		}
		auto& Instances = MainInstances.FindOrAdd(Mains[MainIndex]);
		EnsureInstancesUnique(Transform, OriginalIndex, ComponentName, Instances);
		
		auto& Data = Instances.AddDefaulted_GetRef();
		Data.Id = OriginalIndex;
		Data.ComponentName = ComponentName;
		Data.Transform = Transform;
		
		// Store custom data from component 
		Data.Custom_ActorIndex = ActorIndex;
		Data.Custom_SetIndex = SetIndex;
		
		FBiomesInstanceHandle Result;
		Result.InstanceId = Data.Id;
		Result.GroupId = -(MainIndex + 1);
		Result.ComponentName = ComponentName;
		return Result;
	}

	if (const auto* Actor = Cast<APCGPartitionActor>(Owner))
	{
		const auto ActorGridCoords = Actor->GetGridCoord();
		const auto ActorGridSize = Actor->GetPCGGridSize();

		// Ensure unique
		if (auto* Items = PartitionedInstances.Find({ActorGridCoords, ActorGridSize}))
		{
			EnsureInstancesUnique(Transform, OriginalIndex, ComponentName, *Items);
		}
		
		auto& Data = PartitionedInstances.FindOrAdd({ActorGridCoords, ActorGridSize}).AddDefaulted_GetRef();
		Data.Id = OriginalIndex;
		Data.ComponentName = ComponentName;
		Data.Transform = Transform;

		// Store custom data from component 
		Data.Custom_ActorIndex = ActorIndex;
		Data.Custom_SetIndex = SetIndex;
		
		FBiomesInstanceHandle Result;
		Result.InstanceId = Data.Id;
		Result.GroupId = GetPartitionIndex(ActorGridCoords, ActorGridSize) + 1; 
		Result.ComponentName = ComponentName;
		return Result;
	}
	return {};
}

void UBiomesInstanceController::OnInstanceIndexRelocated(UInstancedStaticMeshComponent* Component,
                                                         TArrayView<const FInstancedStaticMeshDelegates::FInstanceIndexUpdateData> Data)
{
	if (auto* Indices = TrackedComponents.Find(Component))
	{
		checkSlow(Indices.Num() >= Component->GetNumInstances());
		
		for (const auto& Item: Data)
		{
			if (Item.Type == FInstancedStaticMeshDelegates::EInstanceIndexUpdateType::Relocated)
			{
				Indices->Swap(Item.OldIndex, Item.Index);
			}
		}
	}
}

void UBiomesInstanceController::OnInstanceIndexUpdated(UInstancedStaticMeshComponent* Component,
                                                       TArrayView<const FInstancedStaticMeshDelegates::FInstanceIndexUpdateData> Data)
{
	for (const auto& Item: Data)
	{
		if (Item.Type == FInstancedStaticMeshDelegates::EInstanceIndexUpdateType::Relocated)
		{
			if (auto* System = GetInstance(Component))
			{
				System->OnInstanceIndexRelocated(Component, Data);
			}
			return;
		}
	}
}

int16 UBiomesInstanceController::GetMainIndex(const UPCGComponent* Component)
{
	ensure(!Component->IsLocalComponent() && !Component->IsPartitioned());
	
	if (const auto* Manager = Component->GetOwner()->FindComponentByClass<UBiomesSpawnManager>())
	{
		const auto TargetGuid = Manager->Guid;
		ensure(TargetGuid.IsValid());
		for (int16 i = 0; i < Mains.Num(); ++i)
		{
			if (Mains[i] == TargetGuid)
			{
				return i;
			}
		}
	
		return Mains.Add(TargetGuid);
	}
	ensureMsgf(false, TEXT("PCG component should have UBiomesSpawnManager"));
	return INDEX_NONE;
}

int16 UBiomesInstanceController::GetPartitionIndex(const FIntVector& ActorGridCoords, uint32 ActorGridSize)
{
	for (int16 i = 0; i < Partitions.Num(); ++i)
	{
		const auto& Partition = Partitions[i];
		if (Partition.GridSize == ActorGridSize && Partition.GridCoord == ActorGridCoords)
		{
			return i;
		}
	}
	
	return Partitions.Add(FBiomesPartition{ActorGridCoords, ActorGridSize});
}

void UBiomesInstanceController::InitTrackedComponent(const UInstancedStaticMeshComponent* Component, TArray<signed int>& Mapping)
{
	Mapping.SetNumUninitialized(Component->GetNumInstances());
	// Fill the array with initial indices
	for (int i = 0; i < Mapping.Num(); ++i)
	{
		Mapping[i] = i;
	}
}

int32 UBiomesInstanceController::GetOriginalIndex(UInstancedStaticMeshComponent* Component, int32 InstanceId)
{
	auto& Mapping =  TrackedComponents.FindOrAdd(Component);
	if (Mapping.IsEmpty())
	{
		InitTrackedComponent(Component, Mapping);
		
		return InstanceId;
	}
	checkSlow(Mapping.Num() >= Component->GetNumInstances());
	return Mapping[InstanceId];
}

void UBiomesInstanceController::SetOriginalIndex(const UInstancedStaticMeshComponent* Component, int32 OriginalId,
	int32 InstanceId)
{
	auto& Mapping =  TrackedComponents.FindOrAdd(Component);
	if (Mapping.IsEmpty())
	{
		InitTrackedComponent(Component, Mapping);
	}
	checkSlow(Mapping.Num() >= Component->GetNumInstances());
	Mapping[InstanceId] = OriginalId;
}

void UBiomesInstanceController::Track(UInstancedStaticMeshComponent* Component)
{
	auto& Mapping =  TrackedComponents.FindOrAdd(Component);
	if (Mapping.IsEmpty())
	{
		InitTrackedComponent(Component, Mapping);
	}
}

UBiomesInstanceController* UBiomesInstanceController::GetInstance(const UObject* WorldContext)
{
	return WorldContext->GetWorld()->GetSubsystem<UBiomesInstanceController>();
}

FBiomesInstanceHandle UBiomesInstanceController::RemoveInstance(UInstancedStaticMeshComponent* Component,
	int32 InstanceId)
{
	if (!Component->IsValidInstance(InstanceId))
	{
		return {};
	}
	
	if (const auto Handle = RemoveInstanceImpl(Component, InstanceId))
	{
		ensure(Component->RemoveInstance(InstanceId));
		return Handle;
	}
	return {};
}

bool UBiomesInstanceController::RestoreInstanceImpl(const FName& ComponentName, const FBiomesInstanceData& Data, AActor* Actor)
{
	if (UInstancedStaticMeshComponent* Component = FindISM(Actor, ComponentName))
	{
		const auto InstanceId = Component->AddInstance(Data.Transform);
					
		SetOriginalIndex(Component, Data.Id, InstanceId);
					
		UBiomesSpawnManager::SetTagEntryFromInstance(Component, InstanceId,{
			                                             .SetIndex = Data.Custom_SetIndex,
			                                             .ActorIndex = Data.Custom_ActorIndex
		                                             });
					
		return true;
	}
	return false;
}

bool UBiomesInstanceController::RestoreInstance(const FBiomesInstanceHandle& InstanceHandle)
{
	FResult Result = {};
	if (FindDataByHandle(InstanceHandle, Result))
	{
		if (Result.Actor)
		{
			const auto& Data = Result.GetData();

			RestoreInstanceImpl(InstanceHandle.ComponentName, Data, Result.Actor);
		}
		
		Result.Instances->RemoveAtSwap(Result.Index, EAllowShrinking::No);
		return true;
	}
	return false;
}

bool UBiomesInstanceController::FindDataByHandle(const FBiomesInstanceHandle& Handle, FResult& Result)
{
	if (!Handle)
	{
		return false;
	}

	const auto* PCG = UPCGSubsystem::GetSubsystemForCurrentWorld();
	if (!PCG)
	{
		return false;
	}

	if (Handle.GroupId < 0)
	{
		const auto MainIndex = -(Handle.GroupId + 1);
		if (!Mains.IsValidIndex(MainIndex))
		{
			return false;
		}
		auto& Instances = MainInstances.FindOrAdd(Mains[MainIndex]);
		const auto Index = Instances.IndexOfByKey(Handle);
		if (Index == INDEX_NONE)
		{
			return false;
		}

		if (UPCGComponent* Component = FindPcgComponent(Mains[MainIndex]))
		{
			Result.Actor = Component->GetOwner();
		}
		Result.Index = Index;
		Result.Instances = &Instances;
		return true;
	}

	const auto PartitionIndex = Handle.GroupId - 1;
	if (!Partitions.IsValidIndex(PartitionIndex))
	{
		return false;
	}
	const auto& Partition = Partitions[PartitionIndex];
	
	if (auto* Instances = PartitionedInstances.Find(Partition))
	{
		const auto Index = Instances->IndexOfByKey(Handle);
		if (Index == INDEX_NONE)
		{
			return false;
		}

		Result.Actor = PCG->GetRegisteredPCGPartitionActor(Partition.GridSize, Partition.GridCoord, false);
		Result.Index = Index;
		Result.Instances = Instances;
		return true;
	}
	return false;
}

bool UBiomesInstanceController::GetInstanceTransform(const FBiomesInstanceHandle& InstanceHandle, FTransform& InstanceTransform)
{
	FResult Result = {};
	if (FindDataByHandle(InstanceHandle, Result))
	{
		InstanceTransform = Result.GetData().Transform;
		return true;
	}
	return false;
}

UBiomesInstanceUserData* UBiomesInstanceController::GetUserData(const FBiomesInstanceHandle& InstanceHandle)
{
	FResult Result = {};
	if (FindDataByHandle(InstanceHandle, Result))
	{
		if (auto* Manager = UBiomesSpawnManager::GetManager(Result.Actor))
		{
			const auto& Data = Result.GetData();
			return Manager->GetExtraData(Data.Custom_SetIndex, Data.Custom_ActorIndex);
		}
	}
	return nullptr;
}

FBiomesPersistentInstancesData UBiomesInstanceController::GetPersistentData() const
{
	TArray<FBiomesPersistentMainInstances> MainData;
	MainData.Reserve(MainInstances.Num());
	for (const auto& [Guid, Instances]: MainInstances)
	{
		MainData.Add({.Guid = Guid, .Instances = Instances});
	}

	TArray<FBiomesPersistentPartitionedInstances> PartitionedData;
	PartitionedData.Reserve(PartitionedInstances.Num());
	for (const auto& [Partition, Instances]: PartitionedInstances)
	{
		PartitionedData.Add({.Partition = Partition, .Instances = Instances});
	}

	return {
		.Mains = Mains,
		.Partitions = Partitions,
		.MainInstances = MoveTemp(MainData),
		.PartitionedInstances = MoveTemp(PartitionedData)
	};
}

void UBiomesInstanceController::SetPersistentData(const FBiomesPersistentInstancesData& Data)
{
	const auto* PCG = UPCGSubsystem::GetSubsystemForCurrentWorld();

	// Now I am using inefficient approach - restore the state of all partition actors and then remove all from new data.
	// So if these lists are equals, we waste a lot of resources doing nothing
	if (PCG)
	{
		if (!MainInstances.IsEmpty())
		{
			for (const auto& [Guid, Instances]: MainInstances)
			{
				if (const auto* Component = FindPcgComponent(Guid))
				{
					for (const auto& InstanceData: Instances)
					{
						RestoreInstanceImpl(InstanceData.ComponentName, InstanceData, Component->GetOwner());
					}
				}
			}
		}

		if (!PartitionedInstances.IsEmpty())
		{
			for (const auto& [Partition, Instances]: PartitionedInstances)
			{
				if (auto* Actor = PCG->GetRegisteredPCGPartitionActor(Partition.GridSize, Partition.GridCoord, false))
				{
					for (const auto& InstanceData: Instances)
					{
						RestoreInstanceImpl(InstanceData.ComponentName, InstanceData, Actor);
					}
				}
			}
		}
	}

	// Copy new data
	Mains = Data.Mains;
	Partitions = Data.Partitions;

	MainInstances.Empty(Data.MainInstances.Num());
	for (const auto& [Guid, Instances] : Data.MainInstances)
	{
		MainInstances.Add(Guid, Instances);
	}

	PartitionedInstances.Empty(Data.PartitionedInstances.Num());
	for (const auto& [Partition, Instances] : Data.PartitionedInstances)
	{
		PartitionedInstances.Add(Partition, Instances);
	}

	// Remove all new instances
	if (PCG)
	{
		for (const auto& [Guid, Instances] : Data.MainInstances)
		{
			if (const auto* Component = FindPcgComponent(Guid))
			{
				ApplyStateToActor(Component->GetOwner(), Instances, true);
			}
		}

		for (const auto& [Partition, Instances] : Data.PartitionedInstances)
		{
			if (auto* Actor = PCG->GetRegisteredPCGPartitionActor(Partition.GridSize, Partition.GridCoord, false))
			{
				ApplyStateToActor(Actor, Instances, true);
			}
		}
	}
}

struct FInstancesCache
{
	// Mesh can be destroyed, but Groups used only as a local storage and always cleared before access
	TMap<FName, TArray<int32>> Groups;
	UBiomesInstanceController::FTrackedComponents GlobalToLocal;

	void Setup(const TArray<FBiomesInstanceData>& Instances)
	{
		// This is not true - the number of meshes will be less then Num()
		Groups.Empty();
		
		for (const auto& Item : Instances)
		{
			Groups.FindOrAdd(Item.ComponentName).Add(Item.Id);
		}
	}

	void BuildGlobalToLocal(UBiomesInstanceController::FTrackedComponents& TrackedComponents)
	{
		GlobalToLocal.Empty(TrackedComponents.Num());
		
		for (auto It = TrackedComponents.CreateIterator(); It; ++It)
		{
			// Clear dead items from TrackedComponents
			if (!It->Key.IsValid())
			{
				It.RemoveCurrent();
				continue;
			}

			auto& [Component, ToGlobal] = *It;

			// Create reverse mapping - from global to local
			auto& Mapping = GlobalToLocal.FindOrAdd(Component);
			const auto Count = ToGlobal.Num();
			// Limit to current count of elements because ToGlobal might have more elements
			Mapping.SetNumUninitialized(Count, false);
			for (int i = 0; i < Count; ++i)
			{
				const auto GlobalId = ToGlobal[i];
				Mapping[GlobalId] = i;
			}
		}
	}

	void ToLocal(UInstancedStaticMeshComponent* Component, TArray<signed int>& Indices)
	{
		if (auto* Mapping = GlobalToLocal.Find(Component))
		{
			for (auto& GlobalIndex: Indices)
			{
				GlobalIndex = (*Mapping)[GlobalIndex];
			}
		}
	}
};

bool UBiomesInstanceController::ApplyStateToActor(AActor* Actor,
	const FBiomesInstances& Instances, bool ConvertToLocal)
{
	auto& ISMs = CachePartition(Actor);

	static FInstancesCache Cache;
	Cache.Setup(Instances);
	if (ConvertToLocal)
	{
		Cache.BuildGlobalToLocal(TrackedComponents);
	}

	bool Success = true;

	for (auto& [ComponentName, Indices]: Cache.Groups)
	{
		if (auto* Component = FindISM(ComponentName, &ISMs))
		{
			if (ConvertToLocal)
			{
				Cache.ToLocal(Component, Indices);
			}
			Indices.Sort(TGreater<int32>());

			Track(Component);
			
			Success &= Component->RemoveInstances(Indices, true);
		}
	}
	return Success;
}

void UBiomesInstanceController::OnPartitionLoaded(APCGPartitionActor* PartitionActor)
{
	int NumInstances = 0;
	bool Success = true;
	if (const auto* Instances = GetInstancesFor(PartitionActor))
	{
		NumInstances += Instances->Num();
		Success = ApplyStateToActor(PartitionActor, *Instances, false);
	}

	if (!Success)
	{
		const auto Coords = PartitionActor->GetGridCoord();
		UE_LOG(LogBiomes, Warning, TEXT("Failed to restore state of partition: %d, %d, %d. Tried to remove %d instances"),
			Coords.X, Coords.Y, Coords.Z, NumInstances);
	}
}

void UBiomesInstanceController::OnPartitionUnloaded(APCGPartitionActor* PartitionActor)
{
	if (auto* Actor = Cast<APCGPartitionActor>(PartitionActor))
	{
		ISMMapping.Remove(Actor);
	}
}

void UBiomesInstanceController::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (auto* World = GetWorld(); World && World->IsGameWorld())
	{
		DelegateHandle = FInstancedStaticMeshDelegates::OnInstanceIndexUpdated.AddStatic(&UBiomesInstanceController::OnInstanceIndexUpdated);
	}
}

void UBiomesInstanceController::Deinitialize()
{
	Super::Deinitialize();

	if (auto* World = GetWorld(); World && World->IsGameWorld())
	{
		FInstancedStaticMeshDelegates::OnInstanceIndexUpdated.Remove(DelegateHandle);
	}
}

void UBiomesInstanceController::RegisterManager(UBiomesSpawnManager* Manager)
{
	Managers.AddUnique(Manager);
}

void UBiomesInstanceController::UnRegisterManager(UBiomesSpawnManager* Manager)
{
	Managers.Remove(Manager);
}

UInstancedStaticMeshComponent* UBiomesInstanceController::FindISM(const FName& ComponentName, const TArray<TObjectPtr<UInstancedStaticMeshComponent>>* ISMs)
{
	const auto* Item = ISMs->FindByPredicate([&ComponentName] (const UInstancedStaticMeshComponent* Component)
	{
		return ComponentName == Component->GetFName();
	});

	return Item ? *Item : nullptr;
}

UBiomesInstanceController::FBiomesInstances* UBiomesInstanceController::GetInstancesFor(const APCGPartitionActor* PartitionActor)
{
	const auto Size = PartitionActor->GetPCGGridSize();
	const auto Coords = PartitionActor->GetGridCoord();
	
	const auto Partition = FBiomesPartition{ Coords, Size};
	return PartitionedInstances.Find(Partition);
}

UInstancedStaticMeshComponent* UBiomesInstanceController::FindISM(AActor* Actor, const FName& ComponentName)
{
	// Find Actor in cache
	if (auto* ISMs = ISMMapping.Find(Actor))
	{
		return FindISM(ComponentName, &ISMs->Components);
	}

	// Get a list of all ISM from actor
	const auto& ISMs = CachePartition(Actor);
	auto* Result = FindISM(ComponentName, &ISMs);
	return Result;
}

const TArray<TObjectPtr<UInstancedStaticMeshComponent>>& UBiomesInstanceController::CachePartition(
	AActor* Actor)
{
	// Get a list of all ISM from actor
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> ISMs;
	Actor->GetComponents(ISMs);
	
	// Save it to cache
	return ISMMapping.Emplace(Actor, FBiomesISMList {MoveTemp(ISMs)}).Components;
}

