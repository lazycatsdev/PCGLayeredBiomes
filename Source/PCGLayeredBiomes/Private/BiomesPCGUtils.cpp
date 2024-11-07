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



#include "BiomesPCGUtils.h"

#include "BiomesSpawnManager.h"
#include "PCGComponent.h"
#include "PCGSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Runtime/BiomesInstanceController.h"
#include "Runtime/BiomesInstanceTracker.h"

template <typename AttributeType>
static bool GetAttribute(const FPCGPoint& Point, const UPCGMetadata* Metadata, FName AttributeName, AttributeType& Value)
{
	const FPCGMetadataAttributeBase* AttributeBase = Metadata->GetConstAttribute(AttributeName);
	if (!AttributeBase)
	{
		return false;
	}

	if (PCG::Private::IsOfTypes<AttributeType>(AttributeBase->GetTypeId()))
	{
		Value = static_cast<const FPCGMetadataAttribute<AttributeType>*>(AttributeBase)->GetValueFromItemKey(Point.MetadataEntry);
		return true;
	}
	return false;
}

bool UBiomesPCGUtils::ExtractSpawnInfo(const UInstancedStaticMeshComponent* Component, const int32 InstanceId, FPCGSpawnInfo& Result)
{
	if (const auto* Mgr = GetSpawnManager(Component))
	{
		
		 return Mgr->GetSpawnInfoFromInstance(Component, InstanceId, Result);
	}
	return false;
}

bool UBiomesPCGUtils::RemoveInstance(UInstancedStaticMeshComponent* Component, const int32 InstanceId,
	FBiomesInstanceHandle& InstanceHandle, FTransform& InstanceTransform)
{
	check(Component);
	check(Component->IsValidInstance(InstanceId));

	UBiomesInstanceController* Controller = UBiomesInstanceController::GetInstance(Component);
	if (!Controller)
	{
		InstanceHandle = {};
		return false;
	}
	
	if (Component->GetInstanceTransform(InstanceId, InstanceTransform, true))
	{
		FBiomesInstanceHandle Handle = Controller->RemoveInstance(Component, InstanceId);
		InstanceHandle = Handle;
		return Handle.IsValid();
	}
	
	InstanceHandle = {};
	return {};
}

bool UBiomesPCGUtils::RestoreInstance(const FBiomesInstanceHandle& InstanceHandle)
{
	const auto* WorldContext = UPCGSubsystem::GetSubsystemForCurrentWorld();
	if (auto* Controller = UBiomesInstanceController::GetInstance(WorldContext))
	{
		return Controller->RestoreInstance(InstanceHandle);
	}
	return false;
}

bool UBiomesPCGUtils::GetTransformByHandle(const FBiomesInstanceHandle& InstanceHandle, FTransform& InstanceTransform)
{
	const auto* WorldContext = UPCGSubsystem::GetSubsystemForCurrentWorld();
	if (auto* Controller = UBiomesInstanceController::GetInstance(WorldContext))
	{
		return Controller->GetInstanceTransform(InstanceHandle, InstanceTransform);
	}
	return false;
}

UBiomesInstanceUserData* UBiomesPCGUtils::GetUserDataByHandle(const FBiomesInstanceHandle& InstanceHandle)
{
	const auto* WorldContext = UPCGSubsystem::GetSubsystemForCurrentWorld();
	if (auto* Controller = UBiomesInstanceController::GetInstance(WorldContext))
	{
		return Controller->GetUserData(InstanceHandle);
	}
	return nullptr;
}

FBiomesPersistentInstancesData UBiomesPCGUtils::GetPersistentData()
{
	const auto* WorldContext = UPCGSubsystem::GetSubsystemForCurrentWorld();
	if (auto* Controller = UBiomesInstanceController::GetInstance(WorldContext))
	{
		return Controller->GetPersistentData();
	}
	return {};
}

void UBiomesPCGUtils::SetPersistentData(const FBiomesPersistentInstancesData& Data)
{
	const auto* WorldContext = UPCGSubsystem::GetSubsystemForCurrentWorld();
	if (auto* Controller = UBiomesInstanceController::GetInstance(WorldContext))
	{
		Controller->SetPersistentData(Data);
	}
}

UBiomesSpawnManager* UBiomesPCGUtils::GetSpawnManager(const UActorComponent* Component)
{
	if (!Component)
	{
		return nullptr;
	}
	
	const auto* Owner = Component->GetOwner();
	check(Owner);

	return GetSpawnManager(Owner);
}

UBiomesSpawnManager* UBiomesPCGUtils::GetSpawnManager(const AActor* Actor)
{
	auto* PcgComponent = Actor->FindComponentByClass<UPCGComponent>();
	if (!PcgComponent)
	{
		return nullptr;
	}
	const auto* Original = PcgComponent->GetOriginalComponent();
	return Original->GetOwner()->GetComponentByClass<UBiomesSpawnManager>();
}

int32 UBiomesPCGUtils::GetInteger32Attribute(const FPCGPoint& Point, const UPCGMetadata* Metadata, FName AttributeName)
{
	int32 Value = 0;
	GetAttribute<int32>(Point, Metadata, AttributeName, Value);
	return Value;
}

FName UBiomesPCGUtils::GetNameAttribute(const FPCGPoint& Point, const UPCGMetadata* Metadata, FName AttributeName)
{
	FName Value = {};
	GetAttribute<FName>(Point, Metadata, AttributeName, Value);
	return Value;
}

UBiomesInstanceUserData* UBiomesPCGUtils::ExtractUserData(const UInstancedStaticMeshComponent* Component, const int32 InstanceId)
{
	if (auto* Mgr = GetSpawnManager(Component))
	{
		return Mgr->GetExtraDataFromInstance(Component, InstanceId);
	}
	return nullptr;
}
