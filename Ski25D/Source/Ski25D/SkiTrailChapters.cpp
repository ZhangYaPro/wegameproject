#include "SkiSampleTrail.h"
#include "ProceduralMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

namespace
{
    int32 ActiveCourse=0;
    enum { SnowBall,WoodBeam,SnowCube,CedarCube,DarkCube,GoldCube,GlassCube,PineCone,WhiteCone,RockBall,IceCube,FlagCube,SnowBranch,LampBall,GondolaJilin,GondolaBeidahu };
    float InterpolateCourse(float X,const TArray<FVector2D>& Points)
    {
        for(int32 I=1;I<Points.Num();++I)if(X<=Points[I].X)
        {
            const float Length=Points[I].X-Points[I-1].X,T=FMath::Clamp((X-Points[I-1].X)/Length,0.f,1.f);
            auto Tangent=[&](int32 K)->float
            {
                if(K==0||K==Points.Num()-1)return 0;
                const float A=(Points[K].Y-Points[K-1].Y)/(Points[K].X-Points[K-1].X),B=(Points[K+1].Y-Points[K].Y)/(Points[K+1].X-Points[K].X);
                return A*B<=0?0:2*A*B/(A+B);
            };
            return (2*T*T*T-3*T*T+1)*Points[I-1].Y+(T*T*T-2*T*T+T)*Length*Tangent(I-1)+(-2*T*T*T+3*T*T)*Points[I].Y+(T*T*T-T*T)*Length*Tangent(I);
        }
        return Points.Last().Y;
    }
}
float ASkiSampleTrail::FinishX=102000;
void ASkiSampleTrail::SetActiveCourse(int32 Index){ActiveCourse=FMath::Clamp(Index,0,2);FinishX=ActiveCourse==0?102000:ActiveCourse==1?152000:184000;}
int32 ASkiSampleTrail::GetActiveCourse(){return ActiveCourse;}
float ASkiSampleTrail::GetFinishX(){return FinishX;}
FString ASkiSampleTrail::CourseTitle(){return ActiveCourse==0?TEXT("雾凇江岸"):ActiveCourse==1?TEXT("北大湖挑战"):TEXT("长白岩谷");}
FName ASkiSampleTrail::CourseMap(int32 Index){return FName(Index==0?TEXT("/Game/Ski25D/Maps/SkiRimeMap"):Index==1?TEXT("/Game/Ski25D/Maps/SkiBeidahuMap"):TEXT("/Game/Ski25D/Maps/SkiChangbaiMap"));}
float ASkiSampleTrail::ObstacleHeight(int32 Index){return ActiveCourse==1?(Index%3==0?115:90):ActiveCourse==2?(Index%3==2?145:Index%2?100:135):(Index%2?95:135);}
float ASkiSampleTrail::ChapterHeight(float X)
{
    // Each chapter is hand-authored in its own coordinates. Long downhill
    // runs are separated by broad landing bowls and distinct uphill lips.
    static const TArray<FVector2D> Sports={
        {-6000,16000},{0,16000},{3000,15900},{14500,13700},{17000,14300},
        {30500,10400},{33500,11200},{48000,7700},{50800,8480},
        {66400,4350},{69300,5160},{82100,2360},{85300,3220},
        {101200,-1460},{104300,-560},{116600,-3600},{119700,-2750},
        {129200,-5120},{132300,-4400},{144800,-7800},{147700,-7100},
        {150000,-7850},{152000,-7900},{158000,-8000}};
    static const TArray<FVector2D> Forest={
        {-6000,22000},{0,22000},{2500,21950},{7800,20500},{14800,18500},{18000,19350},
        {26200,15800},{32100,14400},{34900,15250},{40200,12700},{48500,9900},{51700,10850},
        {59900,7080},{68400,4660},{71500,5530},{77800,2880},{88500,-210},{91900,830},
        {99400,-2520},{108700,-5400},{112100,-4300},{120500,-7900},{128600,-9900},{131900,-8800},
        {139000,-12500},{149000,-15700},{152300,-14700},{160500,-18200},{170500,-22000},{173900,-20850},
        {180000,-24000},{184000,-24150},{191000,-24300}};
    return InterpolateCourse(X,ActiveCourse==1?Sports:Forest);
}

void ASkiSampleTrail::BuildChapterScenery()
{
    const bool Sports=CourseIndex==1;FRandomStream R(Sports?2702:2703);
    // Snowfield for the resort, dark rocky ravine beneath the forest. Neither
    // chapter reuses the river/rime corridor of chapter one.
    for(int32 Section=3;Section<=4;++Section)
    {
        TArray<FVector> V,N;TArray<int32> T;TArray<FVector2D> UV;
        const int32 Steps=FMath::CeilToInt((FinishX+12000)/100);
        for(int32 I=0;I<=Steps;++I)
        {
            const float X=-6000+I*100;
            for(int32 Side=0;Side<2;++Side)
            {
                const float Y=Section==3?(Side?-4400:-1050):(Side?-7800:-4400);
                const float Drop=Section==3?(Sports?450:Side?950:450):120;
                V.Add(FVector(X,Y,HeightAt(X)-Drop));N.Add(FVector::UpVector);UV.Add(FVector2D(X/900,Y/900));
            }
            if(I<Steps){const int32 A=I*2;T.Append({A,A+2,A+1,A+1,A+2,A+3});}
        }
        Terrain->CreateMeshSection_LinearColor(Section,V,T,N,UV,TArray<FLinearColor>(),TArray<FProcMeshTangent>(),false);
        Terrain->SetMaterial(Section,Terrain->GetMaterial(0));
    }
    RebuildBackdrop();
    if(Sports)
    {
        // Lift towers, a continuous sagging cable and coloured gondolas.
        for(float X=-3000;X<FinishX+5000;X+=4800)
        {
            const float Z=HeightAt(X);
            Branch(DarkCube,FVector(X,-1650,Z-465),FVector(X,-1650,Z+1100),25);
            Branch(GoldCube,FVector(X,-1950,Z+1090),FVector(X,-1250,Z+1090),18);
            FVector Previous(X,-1450,Z+1150);
            for(int32 I=1;I<=16;++I)
            {
                const float T=I/16.f,NextX=X+4800*T;
                const FVector Next(NextX,-1450,FMath::Lerp(Z,HeightAt(X+4800),T)+1150-190*FMath::Sin(PI*T));
                Branch(DarkCube,Previous,Next,3);Previous=Next;
            }
            const float CarX=X+2200,CarZ=FMath::Lerp(Z,HeightAt(X+4800),2200/4800.f)+945;
            Branch(DarkCube,FVector(CarX,-1450,CarZ+130),FVector(CarX,-1450,CarZ-40),5);
            Add(FlagCube,FVector(CarX,-1450,CarZ-180),FVector(4.8,2.8,2.8));
            Add(SnowCube,FVector(CarX,-1450,CarZ-32),FVector(5.0,2.9,.14));
            // Eight alternating local destinations and foods, each occupying the entire panel.
            Add(GondolaJilin+int32((X+3000)/4800)%8,FVector(CarX,-1305,CarZ-180),FVector(4.7,2.7,1),FRotator(0,0,90));
        }
        for(float X=-1500;X<FinishX+3000;X+=620)
        {
            const float Z=HeightAt(X);
            Branch(DarkCube,FVector(X,-530,Z),FVector(X,-530,Z+160),4);
            const int32 FencePool=int32(X/620)%2?FlagCube:DarkCube;
            for(float H:{45.f,125.f})Branch(FencePool,FVector(X,-530,HeightAt(X)+H),FVector(X+480,-530,HeightAt(X+480)+H),4);
            for(int32 K=0;K<6;++K)
            {
                const float A=X+K*80,B=A+80;
                Branch(FencePool,FVector(A,-530,HeightAt(A)+45),FVector(B,-530,HeightAt(B)+125),2);
                Branch(FencePool,FVector(A,-530,HeightAt(A)+125),FVector(B,-530,HeightAt(B)+45),2);
            }
            if(int32(X/620)%5==0)Add(FlagCube,FVector(X,-530,Z+225),FVector(1.25,.1,.75));
            if(int32(X/620)%7==0)Pine(X,-5700,1.4);
        }
        for(float X:{2400.f,35800.f,87500.f,135000.f})
        {
            const float Z=HeightAt(X);
            for(int32 Row=0;Row<4;++Row)
            {
                Add(DarkCube,FVector(X,-2700-Row*260,Z+90+Row*120),FVector(18,2.5,1.2));
                for(int32 Seat=0;Seat<10;++Seat)Add(Seat%2?FlagCube:GoldCube,FVector(X-800+Seat*175,-2700-Row*260,Z+170+Row*120),FVector(.85,.8,.7));
            }
            Add(SnowCube,FVector(X,-3200,Z+750),FVector(21,16,.2));
            for(float Dx:{-950.f,950.f})for(float Y:{-2400.f,-3700.f})Branch(DarkCube,FVector(X+Dx,Y,HeightAt(X+Dx)-465),FVector(X+Dx,Y,Z+750),12);
        }
        // Tall start house and finish timing gantry clearly read as a resort.
        for(float X:{1600.f,FinishX-3500})
        {
            const float Z=HeightAt(X);
            Add(DarkCube,FVector(X,-880,Z+380),FVector(10,5,7.6));
            Add(GlassCube,FVector(X,-618,Z+480),FVector(8,.1,2.5));
            Add(FlagCube,FVector(X,-610,Z+160),FVector(9,.1,1.6));
            Add(SnowCube,FVector(X,-880,Z+780),FVector(11,6,.3));
        }
        for(float X:RampPositions())for(float Side:{-1.f,1.f})
        {
            const float Y=Side*480,Z=HeightAt(X);
            Branch(FlagCube,FVector(X,Y,Z),FVector(X,Y,Z+600),12);
            Add(GoldCube,FVector(X,Y,Z+400),FVector(.25,1.6,2.0));
        }
    }
    else
    {
        BuildVolcanicValley();
    }
    UE_LOG(LogTemp,Display,TEXT("SKI_CHAPTER_SCENERY course=%d length=%.0f obstacles=%d ramps=%d"),CourseIndex+1,(FinishX-200)/100,ObstaclePositions().Num(),RampPositions().Num());
}
