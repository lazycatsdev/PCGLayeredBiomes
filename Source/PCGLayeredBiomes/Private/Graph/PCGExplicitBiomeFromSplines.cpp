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



#include "Graph/PCGExplicitBiomeFromSplines.h"

#include "EngineUtils.h"
#include "ExplicitBiomeActor.h"
#include "PCGComponent.h"
#include "PCGModule.h"
#include "Data/PCGSpatialData.h"
#include "Elements/Metadata/PCGMetadataElementCommon.h"
#include "Helpers/PCGDynamicTrackingHelpers.h"
#include "Helpers/PCGHelpers.h"

#define LOCTEXT_NAMESPACE "PCGExplicitBiomeFromSplines"

UPCGExplicitBiomeFromSplines::UPCGExplicitBiomeFromSplines()
{
	bDisplayModeSettings = false;
	Mode = EPCGGetDataFromActorMode::ParseActorComponents;
}

FPCGElementPtr UPCGExplicitBiomeFromSplines::CreateElement() const
{
	return MakeShared<FPCGExplicitBiomeFromSplines>();
}

TArray<FPCGPinProperties> UPCGExplicitBiomeFromSplines::OutputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;
	PinProperties.Emplace(PCGPinConstants::DefaultOutputLabel, EPCGDataType::Spline);

	return PinProperties;
}

FPCGContext* FPCGExplicitBiomeFromSplines::Initialize(const FPCGDataCollection& InputData, TWeakObjectPtr<UPCGComponent> SourceComponent, const UPCGNode* Node)
{
	FPCGDataFromActorContext* Context = new FPCGDataFromActorContext();
	Context->InputData = InputData;
	Context->SourceComponent = SourceComponent;
	Context->Node = Node;

	return Context;
}

bool FPCGExplicitBiomeFromSplines::ExecuteInternal(FPCGContext* InContext) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGDataFromActorElement::Execute);

	check(InContext);
	FPCGDataFromActorContext* Context = static_cast<FPCGDataFromActorContext*>(InContext);

	const UPCGDataFromActorSettings* Settings = Context->GetInputSettings<UPCGDataFromActorSettings>();
	check(Settings);

	if (!Context->bPerformedQuery)
	{
		TFunction<bool(const AActor*)> BoundsCheck = [](const AActor*) -> bool { return true; };
		const UPCGComponent* PCGComponent = Context->SourceComponent.IsValid() ? Context->SourceComponent.Get() : nullptr;
		const AActor* Self = PCGComponent ? PCGComponent->GetOwner() : nullptr;
		if (Self && Settings->ActorSelector.bMustOverlapSelf)
		{
			// Capture ActorBounds by value because it goes out of scope
			const FBox ActorBounds = PCGHelpers::GetActorBounds(Self);
			BoundsCheck = [ActorBounds, PCGComponent](const AActor* OtherActor) -> bool
			{
				const FBox OtherActorBounds = OtherActor ? PCGHelpers::GetGridBounds(OtherActor, PCGComponent) : FBox(EForceInit::ForceInit);
				return ActorBounds.Intersect(OtherActorBounds);
			};
		}

		Context->FoundActors.Reset();
		
		const auto* World = Context->SourceComponent->GetWorld();
		for (TActorIterator<AActor> It(World, AExplicitBiomeActor::StaticClass()); It; ++It)
		{
			AActor* Actor = *It;
			if (BoundsCheck(Actor))
			{
				Context->FoundActors.Add(Actor);
			}
		}
		
		Context->bPerformedQuery = true;

		if (Context->FoundActors.IsEmpty())
		{
			return true;
		}
	}

	if (Context->bPerformedQuery)
	{
		ProcessActors(Context, Settings, Context->FoundActors);

		// Register dynamic tracking
#if WITH_EDITOR
		FPCGDynamicTrackingHelper::AddSingleDynamicTrackingKey(Context, FPCGSelectionKey(AExplicitBiomeActor::StaticClass()), /*bIsCulled=*/false);
#endif // WITH_EDITOR
	}

	return true;
}

void FPCGExplicitBiomeFromSplines::ProcessActors(FPCGContext* Context, const UPCGDataFromActorSettings* Settings, const TArray<AActor*>& FoundActors) const
{
	for (AActor* Actor : FoundActors)
	{
		ProcessActor(Context, Settings, Actor);
	}
}

void FPCGExplicitBiomeFromSplines::ProcessActor(FPCGContext* Context, const UPCGDataFromActorSettings* Settings, AActor* FoundActor) const
{
	check(Context);
	check(Settings);

	if (!FoundActor || !IsValid(FoundActor))
	{
		return;
	}

	TArray<FPCGTaggedData>& Outputs = Context->OutputData.TaggedData;

	const auto Collection = UPCGComponent::CreateActorPCGDataCollection(FoundActor, Context->SourceComponent.Get(), Settings->GetDataFilter(), true);

	if (const auto* Actor = Cast<AExplicitBiomeActor>(FoundActor))
	{
		for (auto& Item: Collection.TaggedData)
		{
			if (const UPCGSpatialData* SpatialData = Cast<UPCGSpatialData>(Item.Data))
			{
				auto* Attribute = ClearOrCreateAttribute(SpatialData->Metadata, "Biome", Actor->Biome);
				if (!Attribute)
				{
					PCGE_LOG(Error, GraphAndLog, LOCTEXT("ErrorCreatingAttribute", "Error while creating attribute 'Biome'"));
				}
			}
		}
	}

	Outputs += Collection.TaggedData;
}

FPCGMetadataAttributeBase* FPCGExplicitBiomeFromSplines::ClearOrCreateAttribute(UPCGMetadata* Metadata, const FName OutputAttributeName, const FName Value)
{
	check(Metadata);

	return PCGMetadataElementCommon::ClearOrCreateAttribute(Metadata, OutputAttributeName, Value);
}

#undef LOCTEXT_NAMESPACE

