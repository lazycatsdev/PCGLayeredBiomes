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



#include "Graph/LBPCGBiomesNoise.h"
#include "GameFramework/Actor.h"

#include "PCGComponent.h"
#include "PCGContext.h"
#include "PCGPin.h"

#include "Data/PCGPointData.h"
#include "Data/PCGSpatialData.h"
#include "Helpers/PCGAsync.h"
#include "Helpers/PCGHelpers.h"
#include "Helpers/PCGSettingsHelpers.h"
#include "LBBiomesPCGUtils.h"

#define LOCTEXT_NAMESPACE "PCGBiomesNoise"

ULBPCGBiomesNoiseSettings::ULBPCGBiomesNoiseSettings()
{
	ValueTarget.SetPointProperty(EPCGPointProperties::Density);
}

TArray<FPCGPinProperties> ULBPCGBiomesNoiseSettings::InputPinProperties() const
{
	return Super::DefaultPointInputPinProperties();
}

TArray<FPCGPinProperties> ULBPCGBiomesNoiseSettings::OutputPinProperties() const
{
	return Super::DefaultPointOutputPinProperties();
}

FPCGElementPtr ULBPCGBiomesNoiseSettings::CreateElement() const
{
	return MakeShared<FLBPCGBiomesNoise>();
}

namespace PCGBiomesNoise
{
	// the useful ranges here are very small so make input easier
	static const double MAGIC_SCALE_FACTOR = 0.0001;

	inline double Fract(double X)
	{
		return X - FMath::Floor(X);
	}

	inline FVector2D Floor(FVector2D X)
	{
		return FVector2D(FMath::Floor(X.X), FMath::Floor(X.Y));
	}

	inline FInt32Point FloorInt(FVector2D X)
	{
		return FInt32Point(FMath::FloorToInt(X.X), FMath::FloorToInt(X.Y));
	}

	inline FVector2D ToVec2D(FInt32Point V)
	{
		return FVector2D(double(V.X), double(V.Y));
	}

	inline FVector2D Fract(FVector2D V)
	{
		return FVector2D(Fract(V.X), Fract(V.Y));
	}

	inline double ValueHash(FVector2D Position)
	{
		Position = 50.0 * Fract(Position * 0.3183099 + FVector2D(0.71, 0.113));
		return -1.0 + 2.0 * Fract(Position.X * Position.Y * (Position.X + Position.Y));
	}

	inline double Noise2D(FVector2D Position)
	{
		const FVector2D FloorPosition = Floor(Position);
		const FVector2D Fraction = Position - FloorPosition;
		const FVector2D U = Fraction * Fraction * (FVector2D(3.0, 3.0) - 2.0 * Fraction);

		return FMath::Lerp(
			FMath::Lerp(ValueHash(FloorPosition), ValueHash(FloorPosition + FVector2D(1.0, 0.0)), U.X),
			FMath::Lerp(ValueHash(FloorPosition + FVector2D(0.0, 1.0)), ValueHash(FloorPosition + FVector2D(1.0, 1.0)), U.X),
			U.Y
		);
	}

	inline FVector2D MultiplyMatrix2D(FVector2D Point, const FVector2D (&Mat2)[2])
	{
		return Point.X * Mat2[0] + Point.Y * Mat2[1];
	}

	// takes a cell id and produces a pseudo random vector offset
	inline FVector2D VoronoiHash2D(FInt32Point Cell)
	{
		const FVector2D P = ToVec2D(Cell);

		// this is some arbitrary large random scale+rotation+skew
		const FVector2D P2 = FVector2D(
			P.Dot(FVector2D(127.1, 311.7)),
			P.Dot(FVector2D(269.5, 183.3))
		);

		// further scale the results by a big number
		return FVector2D(
			Fract(FMath::Sin(P2.X) * 17.1717) - 0.5,
			Fract(FMath::Sin(P2.Y) * 17.1717) - 0.5
		);
	}

	double CalcFractionalBrownian2D(FVector2D Position, int32 Iterations)
	{
		double Z = 0.5;
		double Result = 0.0;

		// just some fixed random rotation and scale numbers
		const FVector2D RotScale[] = {{1.910673, -0.5910404}, {0.5910404, 1.910673}};

		for (int32 I = 0; I < Iterations; ++I)
		{
			Result += FMath::Abs(Noise2D(Position)) * Z;
			Z *= 0.5;
			Position = MultiplyMatrix2D(Position, RotScale);
		}

		return Result;
	}

	const FVector2D PerlinM[] = {{1.6, 1.2}, {-1.2, 1.6}};

	double CalcPerlin2D(FVector2D Position, int32 Iterations)
	{
		double Value = 0.0;

		double Strength = 1.0;

		for (int32 N = 0; N < Iterations; ++N)
		{
			Strength *= 0.5;
			Value += Strength * Noise2D(Position);
			Position = MultiplyMatrix2D(Position, PerlinM);
		}

		return 0.5 + 0.5 * Value;		
	}

	double ApplyContrast(double Value, double Contrast)
	{
		// early out for default 1.0 contrast, the math should be the same
		if (Contrast == 1.0)
		{
			return Value;
		}

		if (Contrast <= 0.0)
		{
			return 0.5;
		}

		Value = FMath::Clamp(Value, 0.0, 1.0);

		if (Value == 1.0)
		{
			return 1.0;
		}

		return 1.0 / (1.0 + FMath::Pow(Value / (1.0 - Value), -Contrast));
	}

	FLocalCoordinates2D CalcLocalCoordinates2D(const FBox& ActorLocalBox, const FTransform& ActorTransformInverse, FVector2D Scale, const FVector& InPosition)
	{
		if (!ActorLocalBox.IsValid)
		{
			return {};
		}

		const FVector2D LocalPosition = FVector2D(ActorTransformInverse.TransformPosition(InPosition));

		const double LeftDist = LocalPosition.X-ActorLocalBox.Min.X;
		const double RightDist = LocalPosition.X-ActorLocalBox.Max.X;

		const double TopDist = LocalPosition.Y-ActorLocalBox.Min.Y;
		const double BottomDist = LocalPosition.Y-ActorLocalBox.Max.Y;

		FLocalCoordinates2D Result;

		Result.X0 = LeftDist * Scale.X;
		Result.X1 = RightDist * Scale.X;
		Result.Y0 = TopDist * Scale.Y;
		Result.Y1 = BottomDist * Scale.Y;

		Result.FracX = FMath::Clamp(LeftDist / (ActorLocalBox.Max.X - ActorLocalBox.Min.X), 0.0, 1.0);
		Result.FracY = FMath::Clamp(TopDist / (ActorLocalBox.Max.Y - ActorLocalBox.Min.Y), 0.0, 1.0);

		return Result;
	};

	struct FSharedParams
	{
		FPCGContext* Context = nullptr;
		FTransform ActorTransformInverse;
		FBox ActorLocalBox;
		FTransform Transform;

		double Brightness;
		double Contrast;
		int32 Iterations;
		bool bTiling;
	};

	struct FBufferParams
	{
		const UPCGPointData* InputPointData = nullptr;
		UPCGPointData* OutputPointData = nullptr;
	};
	
	template<typename FractalNoiseFunc>
	void DoFractal2D(const FSharedParams& SharedParams, const FBufferParams& BufferParams, const ULBPCGBiomesNoiseSettings& Settings, FractalNoiseFunc&& FractalNoise)
	{
		const TArray<FPCGPoint>& SrcPoints = BufferParams.InputPointData->GetPoints();

		struct FProcessResults
		{
			TArray<FPCGPoint> Points;
			TArray<double> Values;
		};

		FProcessResults Results;

		FPCGAsync::AsyncProcessingOneToOneEx(
			SharedParams.Context ? &SharedParams.Context->AsyncState : nullptr,
			SrcPoints.Num(),
			[&Results, Count = SrcPoints.Num()]()
			{
				// initialize
				Results.Points.SetNumUninitialized(Count);
				Results.Values.SetNumUninitialized(Count);
			},
			[
				&SharedParams,
				&BufferParams,
				&Results,
				&SrcPoints,
				&FractalNoise
			](const int32 ReadIndex, const int32 WriteIndex)
			{
				const FPCGPoint& InPoint = SrcPoints[ReadIndex];
				FPCGPoint& OutPoint = Results.Points[WriteIndex];

				OutPoint = InPoint;

				const FVector PointPos = InPoint.Transform.GetTranslation();

				double Value = 0.0;

				if (SharedParams.bTiling)
				{
					const FLocalCoordinates2D LocalCoords = CalcLocalCoordinates2D(
						SharedParams.ActorLocalBox,
						SharedParams.ActorTransformInverse,
						FVector2D(SharedParams.Transform.GetScale3D()),
						PointPos
					);

					Value = FMath::BiLerp(
						FractalNoise(FVector2D(LocalCoords.X0, LocalCoords.Y0), SharedParams.Iterations),
						FractalNoise(FVector2D(LocalCoords.X1, LocalCoords.Y0), SharedParams.Iterations),
						FractalNoise(FVector2D(LocalCoords.X0, LocalCoords.Y1), SharedParams.Iterations),
						FractalNoise(FVector2D(LocalCoords.X1, LocalCoords.Y1), SharedParams.Iterations),
						LocalCoords.FracX,
						LocalCoords.FracY
					);
				}
				else
				{
					Value = FractalNoise(FVector2D(SharedParams.Transform.TransformPosition(PointPos)), SharedParams.Iterations);
				}

				Value = ApplyContrast(SharedParams.Brightness + Value, SharedParams.Contrast);

				Results.Values[WriteIndex] = Value;
			},
			/* bEnableTimeSlicing */ false
		);

		// now apply these results
		BufferParams.OutputPointData->GetMutablePoints() = MoveTemp(Results.Points);

		ULBBiomesPCGUtils::SetAttributeHelper<double>(BufferParams.OutputPointData, Settings.ValueTarget, Results.Values);
	}
}

bool FLBPCGBiomesNoise::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGNoise::Execute);

	const ULBPCGBiomesNoiseSettings* Settings = Context->GetInputSettings<ULBPCGBiomesNoiseSettings>();
	check(Settings);

	FRandomStream RandomSource(Context->GetSeed());

	const FVector RandomOffset = Settings->RandomOffset * FVector(RandomSource.GetFraction(), RandomSource.GetFraction(), RandomSource.GetFraction());

	PCGBiomesNoise::FSharedParams SharedParams;
	SharedParams.Context = Context;

	if (!Context->SourceComponent.IsValid())
	{
		PCGE_LOG(Error, GraphAndLog, LOCTEXT("NoSourceComponent", "No Source Component."));
		return true;		
	}

	{
		AActor* Actor = Context->SourceComponent->GetOwner();
		if (!Actor)
		{
			PCGE_LOG(Error, GraphAndLog, LOCTEXT("NoActor", "Source Component has no actor"));
			return true;		
		}

		SharedParams.ActorLocalBox = PCGHelpers::GetActorLocalBounds(Actor);
		SharedParams.ActorLocalBox.Min *= Actor->GetTransform().GetScale3D();
		SharedParams.ActorLocalBox.Max *= Actor->GetTransform().GetScale3D();

		const FTransform ActorTransform = FTransform(Actor->GetTransform().Rotator(), Actor->GetTransform().GetTranslation(), FVector::One());
		SharedParams.ActorTransformInverse = ActorTransform.Inverse();
	}

	SharedParams.Transform = FTransform(FRotator::ZeroRotator, RandomOffset, FVector(PCGBiomesNoise::MAGIC_SCALE_FACTOR) * Settings->Scale);
	SharedParams.Brightness = Settings->Brightness;
	SharedParams.Contrast = Settings->Contrast;
	SharedParams.bTiling = Settings->bTiling;
	SharedParams.Iterations = FMath::Max(1, Settings->Iterations); // clamped in meta properties but things will crash if it's < 1
	
	TArray<FPCGTaggedData> Inputs = Context->InputData.GetInputsByPin(PCGPinConstants::DefaultInputLabel);	
	for (const FPCGTaggedData& Input : Inputs)
	{
		PCGBiomesNoise::FBufferParams BufferParams;

		BufferParams.InputPointData = Cast<UPCGPointData>(Input.Data);

		if (!BufferParams.InputPointData)
		{
			PCGE_LOG(Error, GraphAndLog, LOCTEXT("InvalidInputData", "Invalid input data (only supports point data)."));
			continue;
		}

		BufferParams.OutputPointData = NewObject<UPCGPointData>();
		BufferParams.OutputPointData->InitializeFromData(BufferParams.InputPointData);
		Context->OutputData.TaggedData.Add_GetRef(Input).Data = BufferParams.OutputPointData;

		DoFractal2D(SharedParams, BufferParams, *Settings, &PCGBiomesNoise::CalcPerlin2D);
	}

	return true;
}

#undef LOCTEXT_NAMESPACE