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



#include "BiomesSpawnManager.h"

#include "EngineUtils.h"
#include "PCGComponent.h"
#include "PCGSubsystem.h"
#include "Biomes/BiomesSettings.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Grid/PCGPartitionActor.h"
#include "Misc/MapErrors.h"
#include "Misc/UObjectToken.h"
#include "Runtime/BiomesInstanceTracker.h"
#include "Logging/MessageLog.h"
#include "Runtime/BiomesInstanceController.h"

#define LOCTEXT_NAMESPACE "Biomes"

UBiomesSpawnManager* UBiomesSpawnManager::GetManager(UPCGComponent* InComponent)
{
	if (!InComponent)
	{
		return nullptr;		
	}

	const AActor* Actor = InComponent->GetOriginalComponent()->GetOwner();
	if (!Actor)
	{
		return nullptr;		
	}

	return Actor->GetComponentByClass<UBiomesSpawnManager>(); 
}

UBiomesSpawnManager* UBiomesSpawnManager::GetManager(const AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;		
	}

	if (auto* Result = Actor->GetComponentByClass<UBiomesSpawnManager>())
	{
		return Result;
	}

	if (auto* PcgComponent = Actor->GetComponentByClass<UPCGComponent>())
	{
		return GetManager(PcgComponent);
	}
	
	return nullptr;
}

void UBiomesSpawnManager::BeginPlay()
{
	Super::BeginPlay();

	if (auto* Controller = UBiomesInstanceController::GetInstance(this))
	{
		Controller->RegisterManager(this);
	}
}

void UBiomesSpawnManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (auto* Controller = UBiomesInstanceController::GetInstance(this))
	{
		Controller->UnRegisterManager(this);
	}

	Super::EndPlay(EndPlayReason);
}

const TArray<FPCGSpawnInfo>* UBiomesSpawnManager::FindSet(const FString& SetName) const
{
	if (!Preset)
	{
		return nullptr;
	}
	
	for (const auto& Item: Preset->Sets)
	{
		if (Item.Name == SetName)
		{
			return &Item.Actors;
		}
	}
	return nullptr;
}

const FBiomeSettings* UBiomesSpawnManager::FindSettings(FName BiomeName) const
{
	return Biomes ? Biomes->FindSettings(BiomeName) : nullptr;
}

FPCGCrc UBiomesSpawnManager::GetBiomesCrc() const
{
	if (Biomes)
	{
		return Biomes->ComputeCrc();
	}
	return {};
}

FSoftObjectPath UBiomesSpawnManager::GetBiomesSoftPath() const
{
	return FSoftObjectPath(Biomes);
}

FPCGCrc UBiomesSpawnManager::GetSpawnPresetCrc() const
{
	if (Preset)
	{
		return Preset->ComputeCrc();
	}
	return {};
}

FSoftObjectPath UBiomesSpawnManager::GetSpawnPresetSoftPath() const
{
	return FSoftObjectPath(Preset);
}

void UBiomesSpawnManager::PreSave(FObjectPreSaveContext SaveContext)
{
	Super::PreSave(SaveContext);

	if (!SaveContext.IsProceduralSave())
	{
		if (!HasAnyFlags(RF_ArchetypeObject) && !Guid.IsValid())
		{
			Guid = FGuid::NewGuid();
		}
		else if (HasAnyFlags(RF_ArchetypeObject) && Guid.IsValid())
		{
			// Guid should be unique per instance, so ensure it's empty for templates 
			Guid = {};
		}
	}
}

#if WITH_EDITOR

void UBiomesSpawnManager::PostLoad()
{
	Super::PostLoad();

	if (!HasAnyFlags(RF_ArchetypeObject))
	{
		PrepareForRuntimeInteractions();
	}
}

void UBiomesSpawnManager::PrepareForRuntimeInteractions() const
{
	if (auto* PCGSubsystem = UPCGSubsystem::GetSubsystemForCurrentWorld())
	{
		if (!PCGSubsystem->OnComponentGenerationCompleteOrCancelled.IsBoundToObject(this))
		{
			PCGSubsystem->OnComponentGenerationCompleteOrCancelled.AddUObject(this, &UBiomesSpawnManager::OnGenerationDone);
		}
	}
	
	FixAllPCGActors();
}

void UBiomesSpawnManager::FixAllPCGActors() const
{
	// Track all partitioned actors 
	for (TActorIterator<APCGPartitionActor> ActorIt(GetWorld()); ActorIt; ++ActorIt)
	{
		auto* Actor = *ActorIt;
		if (!Actor->FindComponentByClass<UBiomesInstanceTracker>())
		{
			Actor->Modify();

			FName NewComponentName = "BiomesInstanceTracker";

			// Construct the new component and attach as needed
			UActorComponent* NewInstanceComponent = NewObject<UActorComponent>(Actor, UBiomesInstanceTracker::StaticClass(), NewComponentName, RF_Transactional);

			Actor->AddInstanceComponent(NewInstanceComponent);
			NewInstanceComponent->OnComponentCreated();
			NewInstanceComponent->RegisterComponent();
		}
	}
}

void UBiomesSpawnManager::OnGenerationDone(UPCGSubsystem* Subsystem) const
{
	FixAllPCGActors();
}

void UBiomesSpawnManager::CheckForErrors()
{
	Super::CheckForErrors();

	if (Preset && Preset->HasUserData())
	{
		auto* Owner = GetOwner();
		
		for (TActorIterator<APCGPartitionActor> ActorIt(GetWorld()); ActorIt; ++ActorIt)
		{
			auto* Actor = *ActorIt;
			if (!Actor->FindComponentByClass<UBiomesInstanceTracker>())
			{
				FMessageLog("MapCheck").Warning()
					->AddToken(FUObjectToken::Create(Owner))
					->AddToken(FTextToken::Create(LOCTEXT(
						"MapCheck_Message_ActorsNotReady",
						"Not all of PCG actors ready for runtime interaction" )))
					->AddToken(FActionToken::Create(LOCTEXT("MapCheck_Message_ActorsNotReady_Fix", "Fix actors"),
						LOCTEXT("MapCheck_Message_ActorsNotReady_Fix_Desc", "Modify PCG actors to be ready"),
						FOnActionTokenExecuted::CreateUObject(this, &UBiomesSpawnManager::PrepareForRuntimeInteractions)
						));
				
				break;
			}
		}
	}
}

#endif

const FPCGSpawnInfo* UBiomesSpawnManager::FindActorInfoByMesh(const TSoftObjectPtr<UStaticMesh>& Mesh, FPackedTagsEntry& OutEntry) const
{
	OutEntry.SetIndex = 0;
	
	for (const auto& [_, Actors]: Preset->Sets)
	{
		OutEntry.ActorIndex = 0;
		for (const auto& Item: Actors)
		{
			if (Item.Mesh == Mesh)
			{
				return &Item;
			}
			OutEntry.ActorIndex++;
		}
		OutEntry.SetIndex++;
	}
	return nullptr;
}

UBiomesInstanceUserData* UBiomesSpawnManager::GetExtraDataFromInstance(const UInstancedStaticMeshComponent* Component,
                                                                       const int32 InstanceId) const
{
	const auto TagEntry = GetTagEntryFromInstance(Component, InstanceId);
	if (TagEntry.ActorIndex != INDEX_NONE && TagEntry.SetIndex != INDEX_NONE)
	{
		return Preset->Sets[TagEntry.SetIndex].Actors[TagEntry.ActorIndex].UserData; 
	}
	return nullptr;
}

UBiomesInstanceUserData* UBiomesSpawnManager::GetExtraData(int32 SetIndex, int32 ActorIndex) const
{
	if (ActorIndex != INDEX_NONE && SetIndex != INDEX_NONE)
	{
		return Preset->Sets[SetIndex].Actors[ActorIndex].UserData; 
	}
	return nullptr;
}

bool UBiomesSpawnManager::GetSpawnInfoFromInstance(
	const UInstancedStaticMeshComponent* Component,
	const int32 InstanceId,
	FPCGSpawnInfo& Result) const
{
	const auto TagEntry = GetTagEntryFromInstance(Component, InstanceId);
	if (TagEntry.ActorIndex != INDEX_NONE && TagEntry.SetIndex != INDEX_NONE)
	{
		Result = Preset->Sets[TagEntry.SetIndex].Actors[TagEntry.ActorIndex];
		return true;
	}
	return false;
}

UBiomesSpawnManager::FPackedTagsEntry UBiomesSpawnManager::GetTagEntryFromInstance(const UInstancedStaticMeshComponent* Component, const int32 InstanceId)
{
	if (Component->PerInstanceSMCustomData.IsEmpty())
	{
		return {};
	}
	
	const auto& Data = Component->PerInstanceSMCustomData;
	const auto Offset = Component->NumCustomDataFloats * InstanceId;
	if (Data.Num() >= Offset + Component->NumCustomDataFloats)
	{
		return *(FPackedTagsEntry*)&Data[Offset];
	}

	return {};
}

void UBiomesSpawnManager::SetTagEntryFromInstance(UInstancedStaticMeshComponent* Component,
	const int32 InstanceId, const FPackedTagsEntry& Data)
{
	if (Component->PerInstanceSMCustomData.IsEmpty())
	{
		return;
	}
	
	auto& CustomData = Component->PerInstanceSMCustomData;
	const auto Offset = Component->NumCustomDataFloats * InstanceId;
	if (CustomData.Num() >= Offset + Component->NumCustomDataFloats)
	{
		FMemory::Memcpy(&CustomData[Offset], &Data, sizeof(FPackedTagsEntry));
	}
}


#undef LOCTEXT_NAMESPACE
