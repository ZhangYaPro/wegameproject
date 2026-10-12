#include "SkiSampleTrail.h"
#include "ProceduralMeshComponent.h"

bool ASkiSampleTrail::IsRideableObstacle(int32 Index){return ObstacleHeight(Index)<=100;}
float ASkiSampleTrail::ObstacleHalfWidth(int32 Index)
{
    return GetActiveCourse()==1?75.f:ObstacleHeight(Index)*.5f;
}
float ASkiSampleTrail::ObstacleTopAt(int32 Index,float X)
{
    const float Center=ObstaclePositions()[Index],H=ObstacleHeight(Index);
    if(GetActiveCourse()==1)return HeightAt(Center)+H;
    const float U=FMath::Clamp((X-Center)/ObstacleHalfWidth(Index),-1.f,1.f);
    return HeightAt(Center)+H*.48f+H*.5f*FMath::Sqrt(FMath::Max(0.f,1-U*U));
}
float ASkiSampleTrail::SurfaceHeightAt(float X)
{
    float Z=HeightAt(X);const auto& Obstacles=ObstaclePositions();
    for(int32 I=0;I<Obstacles.Num();++I)
        if(IsRideableObstacle(I)&&FMath::Abs(X-Obstacles[I])<=ObstacleHalfWidth(I))
            Z=FMath::Max(Z,ObstacleTopAt(I,X));
    return Z;
}
void ASkiSampleTrail::BuildRideableSurfaces()
{
    // Remove the obsolete visible snow ramps, including those serialized in old maps.
    Terrain->ClearMeshSection(8);
    ObstacleTops->ClearAllMeshSections();
    TArray<FVector> V,N;TArray<int32> T;TArray<FVector2D> UV;
    const auto& Obstacles=ObstaclePositions();
    for(int32 I=0;I<Obstacles.Num();++I)if(IsRideableObstacle(I))
    {
        const int32 Start=V.Num();
        const float Width=ObstacleHalfWidth(I),Y=GetActiveCourse()==1?170.f:175.f;
        for(int32 Step=0;Step<=16;++Step)
        {
            const float X=Obstacles[I]+Width*(Step/8.f-1);
            for(float Side:{-Y,Y})
            {
                V.Add(FVector(X,Side,ObstacleTopAt(I,X)));
                N.Add(FVector::UpVector);UV.Add(FVector2D::ZeroVector);
            }
            if(Step<16){const int32 A=Start+Step*2;T.Append({A,A+1,A+2,A+1,A+3,A+2});}
        }
    }
    // Invisible collision only: the original wood/block mesh is the visible surface.
    ObstacleTops->CreateMeshSection_LinearColor(0,V,T,N,UV,TArray<FLinearColor>(),TArray<FProcMeshTangent>(),true);
}
