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



#include "Graph/PCGMeshFromSpawnManager.h"

#include "Engine/StaticMesh.h"
#include "BiomesPCGUtils.h"
#include "PCGComponent.h"
#include "PCGModule.h"
#include "PCGPin.h"
#include "PCGSpawnStructures.h"
#include "BiomesSpawnManager.h"
#include "RandomUtils.h"
#include "Data/PCGPointData.h"
#include "Helpers/PCGAsync.h"
#include "Helpers/PCGDynamicTrackingHelpers.h"
#include "Helpers/PCGHelpers.h"



#define LOCTEXT_NAMESPACE "PCGFPCGMeshFromSpawnManager"

UPCGMeshFromSpawnManagerSettings::UPCGMeshFromSpawnManagerSettings()
{
	ValueTarget.SetAttributeName(TEXT("Mesh"));
	bUseSeed = true;
}

TArray<FPCGPinProperties> UPCGMeshFromSpawnManagerSettings::InputPinProperties() const
{
	return DefaultPointInputPinProperties();
}

TArray<FPCGPinProperties> UPCGMeshFromSpawnManagerSettings::OutputPinProperties() const
{
	return DefaultPointOutputPinProperties();
}

FPCGElementPtr UPCGMeshFromSpawnManagerSettings::CreateElement() const
{
	return MakeShared<FPCGMeshFromSpawnManager>();
}

namespace PCGMeshSet
{
	struct FSharedParams
	{
		FPCGContext* Context = nullptr;
		const TArray<FPCGSpawnInfo>* Actors = nullptr;
		int TotalWeight = 0;
		int32 Seed = 0;
	};

	struct FBufferParams
	{
		const UPCGPointData* InputPointData = nullptr;
		UPCGPointData* OutputPointData = nullptr;
	};

	const FPCGSpawnInfo& SelectRandom(const FSharedParams& SharedParams, const FRandomStream& RandomSource)
	{
		if (SharedParams.TotalWeight <= 1)
		{
			return SharedParams.Actors->Last();	
		}
		
		const auto RandomWeight = RandomSource.RandRange(0, SharedParams.TotalWeight - 1);
		int CurWeight = 0;
		for (const auto& Item: *SharedParams.Actors)
		{
			CurWeight += Item.Weight;
			if (CurWeight > RandomWeight)
			{
				return Item;
			}
		}

		return SharedParams.Actors->Last();
	}
	
	void ProcessPoints(const FSharedParams& SharedParams, const FBufferParams& BufferParams, const UPCGMeshFromSpawnManagerSettings& Settings)
	{
		const TArray<FPCGPoint>& SrcPoints = BufferParams.InputPointData->GetPoints();

		struct FProcessResults
		{
			TArray<FPCGPoint> Points;
			TArray<FString> Values;
			TArray<FVector> BoundsMin;
			TArray<FVector> BoundsMax;
		};

		FProcessResults Results;

		const bool ApplyBounds = Settings.ApplyMeshBounds;
		
		FPCGAsync::AsyncProcessingOneToOneEx(
			SharedParams.Context ? &SharedParams.Context->AsyncState : nullptr,
			SrcPoints.Num(),
			[&Results, Count = SrcPoints.Num(), ApplyBounds]()
			{
				// initialize
				Results.Points.SetNumUninitialized(Count);
				Results.Values.SetNum(Count);
				if (ApplyBounds)
				{
					Results.BoundsMin.SetNum(Count);
					Results.BoundsMax.SetNum(Count);
				}
			},
			[
				&SharedParams,
				&Results,
				&SrcPoints,
				ApplyBounds
			](const int32 ReadIndex, const int32 WriteIndex)
			{
				const FPCGPoint& InPoint = SrcPoints[ReadIndex];
				FPCGPoint& OutPoint = Results.Points[WriteIndex];

				OutPoint = InPoint;
				FRandomStream RandomSource(PCGHelpers::ComputeSeed(SharedParams.Seed, InPoint.Seed));

				const auto& Info = FRandomUtils::SelectRandom<FPCGSpawnInfo>(*SharedParams.Actors, RandomSource, &SharedParams.TotalWeight);

				Results.Values[WriteIndex] = Info.Mesh.ToString();

				if (ApplyBounds && Info.Mesh)
				{
					const auto Bounds = Info.Mesh->GetBounds();
					Results.BoundsMin[WriteIndex] = Bounds.GetBox().Min;
					Results.BoundsMax[WriteIndex] = Bounds.GetBox().Max;
				}
				else if (ApplyBounds)
				{
					Results.BoundsMin[WriteIndex] = FVector::ZeroVector;
					Results.BoundsMax[WriteIndex] = FVector::ZeroVector;
				}
			},
			/* bEnableTimeSlicing */ false
		);

		// now apply these results
		BufferParams.OutputPointData->GetMutablePoints() = MoveTemp(Results.Points);

		UBiomesPCGUtils::SetAttributeHelper<FString>(BufferParams.OutputPointData, Settings.ValueTarget, Results.Values);

		if (ApplyBounds)
		{
			FPCGAttributePropertySelector BoundsMinSelector, BoundsMaxSelector;
			BoundsMinSelector.SetPointProperty(EPCGPointProperties::BoundsMin);
			BoundsMaxSelector.SetPointProperty(EPCGPointProperties::BoundsMax);
		
			UBiomesPCGUtils::SetAttributeHelper<FVector>(BufferParams.OutputPointData, BoundsMinSelector, Results.BoundsMin);
			UBiomesPCGUtils::SetAttributeHelper<FVector>(BufferParams.OutputPointData, BoundsMaxSelector, Results.BoundsMax);
		}
	}
}

bool FPCGMeshFromSpawnManager::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGNoise::Execute);

	const auto* Settings = Context->GetInputSettings<UPCGMeshFromSpawnManagerSettings>();
	check(Settings);

	PCGMeshSet::FSharedParams SharedParams;
	SharedParams.Context = Context;
	SharedParams.Seed = Context->GetSeed();
	
	if (Settings->SetName.IsEmpty())
	{
		return true;
	}
	
	const auto* Manager = UBiomesSpawnManager::GetManager(Context->SourceComponent.Get()); 
	if (!Manager)
	{
		PCGE_LOG(Error, GraphAndLog, LOCTEXT("NoActorsManager", "Source Actor has no UBiomesSpawnManager component"));
		return true;		
	}
	
	SharedParams.Actors = Manager->FindSet(Settings->SetName);
	if (!SharedParams.Actors)
	{
		PCGE_LOG(Error, GraphAndLog, LOCTEXT("NoSet", "Set not found in UBiomesSpawnManager component"));
		return true;		
	}

	if (SharedParams.Actors->IsEmpty())
	{
		return true;		
	}

	// Calculate Total Weight
	SharedParams.TotalWeight = 0;
	for (const auto& Item: *SharedParams.Actors)
	{
		SharedParams.TotalWeight += Item.Weight;
	}

	if (SharedParams.TotalWeight == 0)
	{
		PCGE_LOG(Warning, GraphAndLog, LOCTEXT("WrongWeights", "All meshes in set has 0 weight - it's not supported"));
		return true;
	}
	
	TArray<FPCGTaggedData> Inputs = Context->InputData.GetInputsByPin(PCGPinConstants::DefaultInputLabel);	
	for (const FPCGTaggedData& Input : Inputs)
	{
		PCGMeshSet::FBufferParams BufferParams;

		BufferParams.InputPointData = Cast<UPCGPointData>(Input.Data);

		if (!BufferParams.InputPointData)
		{
			PCGE_LOG(Error, GraphAndLog, LOCTEXT("InvalidInputData", "Invalid input data (only supports point data)."));
			continue;
		}

		BufferParams.OutputPointData = NewObject<UPCGPointData>();
		BufferParams.OutputPointData->InitializeFromData(BufferParams.InputPointData);
		Context->OutputData.TaggedData.Add_GetRef(Input).Data = BufferParams.OutputPointData;

		PCGMeshSet::ProcessPoints(SharedParams, BufferParams, *Settings);
	}

	// Register dynamic tracking
#if WITH_EDITOR
	FPCGDynamicTrackingHelper::AddSingleDynamicTrackingKey(Context,
		FPCGSelectionKey::CreateFromPath(Manager->GetSpawnPresetSoftPath()), /*bIsCulled=*/false);
#endif // WITH_EDITOR

	return true;
}

void FPCGMeshFromSpawnManager::GetDependenciesCrc(const FPCGDataCollection& InInput, const UPCGSettings* InSettings,
	UPCGComponent* InComponent, FPCGCrc& OutCrc) const
{
	FPCGCrc Crc;
	FPCGPointProcessingElementBase::GetDependenciesCrc(InInput, InSettings, InComponent, Crc);

	const auto* Manager = UBiomesSpawnManager::GetManager(InComponent);
	if (!Manager)
	{
		OutCrc = Crc;
		return;		
	}

	Crc.Combine(Manager->GetSpawnPresetCrc());
	
	OutCrc = Crc;
}


#undef LOCTEXT_NAMESPACE
