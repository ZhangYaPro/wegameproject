#include "SkiSampleTrail.h"
#include "ProceduralMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Materials/MaterialInterface.h"

void ASkiSampleTrail::BuildVolcanicValley()
{
    enum {SnowBall,WoodBeam,SnowCube,CedarCube,DarkCube,GoldCube,GlassCube,PineCone,WhiteCone,RockBall,IceCube};
    if(auto* Ice=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Ski25D/Materials/M_GlacierIce.M_GlacierIce")))Pools[IceCube]->SetMaterial(0,Ice);
    // Broad connected ridges behind the trail; no close, repeating pillars.
    const float Y[]={-2300,-2900,-3600,-4500,-5800,-7800};
    const int32 Steps=FMath::CeilToInt((FinishX+12000)/140.f);
    auto Crest=[](float X){return 180+260*FMath::Square(FMath::Sin(X*.00019f))+80*FMath::Sin(X*.00091f);};
    auto Height=[&](float X,int32 J)
    {
        const float C=Crest(X),Z[]={-650,-200,C*.65f,C,C*.45f,-120};return HeightAt(X)+Z[J];
    };
    for(int32 Layer=0;Layer<2;++Layer)
    {
        TArray<FVector> V,N;TArray<int32> T;TArray<FVector2D> UV;
        const int32 Begin=0,End=Layer?5:6,Columns=End-Begin;
        for(int32 I=0;I<=Steps;++I)
        {
            const float X=-6000+I*140;
            for(int32 J=Begin;J<End;++J)
            {
                const float SnowEdge=0;
                V.Add(FVector(X,Y[J]+SnowEdge,Height(X,J)+(Layer?12:0)));N.Add(FVector::ZeroVector);UV.Add(FVector2D(X/1800,Y[J]/1800));
                if(I<Steps&&J<End-1){const int32 A=I*Columns+J-Begin,B=A+Columns;T.Append({A,B,A+1,A+1,B,B+1});}
            }
        }
        for(int32 I=0;I<T.Num();I+=3)
        {
            const int32 A=T[I],B=T[I+1],C=T[I+2];const FVector Normal=FVector::CrossProduct(V[C]-V[A],V[B]-V[A]);N[A]+=Normal;N[B]+=Normal;N[C]+=Normal;
        }
        for(FVector& Normal:N)Normal.Normalize();
        Terrain->CreateMeshSection_LinearColor(6+Layer,V,T,N,UV,TArray<FLinearColor>(),TArray<FProcMeshTangent>(),false);
        Terrain->SetMaterial(6+Layer,Layer?Terrain->GetMaterial(0):Pools[RockBall]->GetMaterial(0));
    }
    FRandomStream R(8327);
    for(float X=4500;X<FinishX;X+=9400)
    {
        for(float Dx:{-180.f,100.f,330.f})
        {
            const float Px=X+Dx,Base=HeightAt(Px)-200+(200+Crest(Px)*.65f)/7;
            Add(RockBall,FVector(Px,-3000,Base+65),FVector(3.4,2.4,2.5),FRotator(7,Dx*.1f,9));
            Add(SnowBall,FVector(Px,-3000,Base+178),FVector(3.2,2.3,.4));
        }
    }
    for(float X=-2200;X<FinishX+2500;X+=2400+R.FRandRange(0,2300))
    {
        const float Z=HeightAt(X),Size=R.FRandRange(1.4,3.0);
        Add(RockBall,FVector(X,-1350,Z-415),FVector(Size*2.4,Size*1.8,Size*2),FRotator(18,R.FRandRange(-30,30),0));
        Add(SnowBall,FVector(X,-1350,Z-415+Size*84),FVector(Size*2.0,Size*1.7,.35));
    }
    // Occasional ice cascades sit in the mid-distance rather than filling
    // every screen. Vary length and width, taper their bases into the valley.
    for(float X:{17500.f,56300.f,98600.f,147000.f})
    {
        for(int32 J=-4;J<=4;++J)
        {
            const float Px=X+J*130,Top=HeightAt(Px)+Crest(Px)*.67f,Bottom=HeightAt(Px)-260;
            Add(IceCube,FVector(Px,-3540,(Top+Bottom)*.5),FVector(1.45,.8,(Top-Bottom)/100),FRotator(-2,0,J*.6f));
            Add(SnowBall,FVector(Px,-3550,Top+8),FVector(1.65,1.5,.22));
        }
    }
    // Sparse trees give scale; distant scenery stays visibly open.
    for(float X=5000;X<FinishX;X+=17300){Pine(X,-6100,.95);Pine(X+430,-6250,.75);}
    for(float X:{36200.f,113500.f})
    {
        const float Z=HeightAt(X);
        Add(DarkCube,FVector(X,-820,Z+190),FVector(5.8,4.5,3.8));
        Add(GlassCube,FVector(X,-590,Z+240),FVector(4.7,.10,1.1));
        Add(SnowCube,FVector(X,-820,Z+410),FVector(6.4,5.1,.35));
    }
}
