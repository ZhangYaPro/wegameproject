#include "SkiSampleTrail.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    enum { SnowBall, WoodBeam, SnowCube, CedarCube, DarkCube, GoldCube, GlassCube,
           PineCone, WhiteCone, RockBall, IceCube, FlagCube, SnowBranch, LampBall, GondolaJilin, GondolaBeidahu,
           GondolaJingyuetan,GondolaChagan,GondolaColdNoodles,GondolaRiceCake,GondolaGinseng,GondolaKimchi,PoolCount };
}

ASkiSampleTrail::ASkiSampleTrail()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Terrain = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("SnowTerrain"));
    Terrain->SetupAttachment(RootComponent);
    Terrain->bUseAsyncCooking = false;
    Terrain->SetCollisionProfileName(TEXT("BlockAll"));
    ObstacleTops=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ObstacleTops"));
    ObstacleTops->SetupAttachment(RootComponent);
    ObstacleTops->bUseAsyncCooking=false;
    ObstacleTops->SetCollisionObjectType(ECC_GameTraceChannel1);
    ObstacleTops->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ObstacleTops->SetCollisionResponseToAllChannels(ECR_Ignore);
    ObstacleTops->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block);
    ObstacleTops->SetVisibility(false);
    ObstacleTops->SetHiddenInGame(true);
    const TCHAR* Shapes[] = {TEXT("Sphere"),TEXT("Cylinder"),TEXT("Cube"),TEXT("Cube"),TEXT("Cube"),TEXT("Cube"),TEXT("Cube"),TEXT("Cone"),TEXT("Cone"),TEXT("Sphere"),TEXT("Cube"),TEXT("Cube"),TEXT("Cylinder"),TEXT("Sphere"),TEXT("Plane"),TEXT("Plane")};
    const TCHAR* Mats[] = {TEXT("Snow"),TEXT("Bark"),TEXT("Snow"),TEXT("Cedar"),TEXT("Midnight"),TEXT("Brass"),TEXT("WarmGlass"),TEXT("Pine"),TEXT("Snow"),TEXT("Stone"),TEXT("River"),TEXT("Coral"),TEXT("Snow"),TEXT("LampGlow"),TEXT("GondolaJilin"),TEXT("GondolaBeidahu"),TEXT("GondolaJingyuetan"),TEXT("GondolaChagan"),TEXT("GondolaColdNoodles"),TEXT("GondolaRiceCake"),TEXT("GondolaGinseng"),TEXT("GondolaKimchi")};
    for (int32 I=0; I<PoolCount; ++I)
    {
        auto* Pool = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(*FString::Printf(TEXT("Scenery%02d"),I));
        Pool->SetupAttachment(RootComponent);
        const TCHAR* Shape=I>=GondolaJilin?TEXT("Plane"):Shapes[I];
        ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(*FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"),Shape,Shape));
        Pool->SetStaticMesh(Mesh.Object);
        ConstructorHelpers::FObjectFinder<UMaterialInterface> Mat(*FString::Printf(TEXT("/Game/Ski25D/Materials/M_%s.M_%s"),Mats[I],Mats[I]));
        if (Mat.Succeeded()) Pool->SetMaterial(0,Mat.Object);
        Pool->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Pool->SetCullDistances(0,22000);
        Pools.Add(Pool);
    }
    // Scenery remains nonblocking. Bare obstacle tops have a separate collision
    // channel; frontal impacts use the swept gameplay check in the character.
    ConstructorHelpers::FObjectFinder<UMaterialInterface> Snow(TEXT("/Game/Ski25D/Materials/M_Snow.M_Snow"));
    ConstructorHelpers::FObjectFinder<UMaterialInterface> Edge(TEXT("/Game/Ski25D/Materials/M_IceEdge.M_IceEdge"));
    if (Snow.Succeeded()) Terrain->SetMaterial(0,Snow.Object);
    if (Edge.Succeeded()) Terrain->SetMaterial(1,Edge.Object);
}

// Actors that are loaded from disk (i.e. every placed actor in a packaged build)
// do NOT go through OnConstruction: per Epic's Actor Lifecycle documentation the
// "Load From Disk" path is PostLoad -> PreInitializeComponents ->
// PostInitializeComponents -> BeginPlay, and ExecuteConstruction/OnConstruction
// only happens on the "Spawn" and "Play In Editor" paths. All of the visible
// scenery lives in the HISM pools filled by OnConstruction, so without this the
// packaged game keeps the serialized terrain and obstacle collision but renders
// no trees, obstacles, lodges, lamps or gondolas.
// The generation is deterministic (fixed random seeds), so rebuilding here gives
// exactly the same result as the editor.
void ASkiSampleTrail::BeginPlay()
{
    Super::BeginPlay();
    RebuildCourse();
}

float ASkiSampleTrail::HeightAt(float X)
{
    if(GetActiveCourse()>0)return ChapterHeight(X);
    const FVector2D Points[] = {
        {-3000,14000},{0,14000},{1600,13960},{5800,12750},{10500,11200},
        {11900,11550},{15800,9910},{19500,8500},{21100,8700},
        {24500,7600},{25700,7970},{30800,5650},{36800,3850},
        {38700,4370},{42400,3000},{43700,3230},{50600,480},{56800,-1900},
        {58800,-1400},{63200,-2950},{64900,-2500},{68900,-4220},{72800,-5850},
        {74600,-5330},{81200,-8370},{87800,-11400},{89900,-10820},
        {93500,-13100},{96900,-15600},{98100,-15300},{100900,-16800},
        {102000,-16800},{108000,-17000}};
    for (int32 I=1; I<UE_ARRAY_COUNT(Points); ++I)
    {
        if (X<=Points[I].X)
        {
            const float T=FMath::Clamp((X-Points[I-1].X)/(Points[I].X-Points[I-1].X),0.f,1.f);
            // Monotone Hermite slopes keep successive downhill sections flowing.
            // True crests/valleys stay horizontal; intermediate knots no longer
            // flatten the skis and interrupt the descent every few metres.
            auto Tangent=[&](int32 K)->float
            {
                if(K==0||K==UE_ARRAY_COUNT(Points)-1)return 0;
                const float Left=(Points[K].Y-Points[K-1].Y)/(Points[K].X-Points[K-1].X);
                const float Right=(Points[K+1].Y-Points[K].Y)/(Points[K+1].X-Points[K].X);
                if(Left*Right<=0)return 0;
                return 2*Left*Right/(Left+Right);
            };
            const float Length=Points[I].X-Points[I-1].X;
            return (2*T*T*T-3*T*T+1)*Points[I-1].Y+(T*T*T-2*T*T+T)*Length*Tangent(I-1)
                +(-2*T*T*T+3*T*T)*Points[I].Y+(T*T*T-T*T)*Length*Tangent(I);
        }
    }
    return Points[UE_ARRAY_COUNT(Points)-1].Y;
}

const TArray<float>& ASkiSampleTrail::ObstaclePositions()
{
    static const TArray<float> Sports={6500,13000,23000,29800,39000,46900,54000,64700,73900,80100,92300,101300,112900,123300,137600,145000};
    static const TArray<float> Forest={6200,14100,23000,31800,39500,48100,58200,67400,76000,87100,97700,108200,118500,128300,138100,148700,159700,171000,176600};
    if(GetActiveCourse()==1)return Sports;
    if(GetActiveCourse()==2)return Forest;
    static const TArray<float> Positions = {3600,8700,15100,18400,21600,31100,35400,40100,46500,
        53900,61700,68900,72100,79200,84500,93600,96500};
    return Positions;
}

const TArray<float>& ASkiSampleTrail::RampPositions()
{
    static const TArray<float> Sports={16800,33300,50600,69100,85100,104100,119500,132100,147500};
    static const TArray<float> Forest={17800,34700,51500,71300,91700,111900,131700,152100,173700};
    if(GetActiveCourse()==1)return Sports;
    if(GetActiveCourse()==2)return Forest;
    static const TArray<float> Positions = {11800,21000,25600,38600,43600,58700,64800,74500,89800,98000};
    return Positions;
}

void ASkiSampleTrail::Add(int32 Pool,FVector Position,FVector Scale,FRotator Rotation)
{
    Pools[Pool]->AddInstance(FTransform(Rotation,Position,Scale),false);
}

void ASkiSampleTrail::Branch(int32 Pool,const FVector& A,const FVector& B,float Radius)
{
    const FVector Delta=B-A;
    const FQuat Q=FQuat::FindBetweenNormals(FVector::UpVector,Delta.GetSafeNormal());
    Pools[Pool]->AddInstance(FTransform(Q,(A+B)*.5f,FVector(Radius/50,Radius/50,Delta.Size()/100)),false);
}

void ASkiSampleTrail::RimeTree(float X,float Y,float S,FRandomStream& R)
{
    const FVector Base(X,Y,HeightAt(X)-35);
    const float H=R.FRandRange(620,900)*S;
    Branch(WoodBeam,Base,Base+FVector(20*S,0,H),19*S);
    Branch(SnowBranch,Base+FVector(0,9*S,60*S),Base+FVector(20*S,9*S,H),10*S);
    for(int32 Level=0;Level<4;++Level)
    {
        const float Z=H*(.26f+Level*.17f);
        for(int32 K=0;K<4;++K)
        {
            const float Angle=K*PI*.5f+Level*.67f+R.FRandRange(-.2,.2);
            const FVector Dir(FMath::Cos(Angle),FMath::Sin(Angle),.5f);
            const FVector Start=Base+FVector(0,0,Z);
            const float L=(260-Level*37)*S;
            const FVector End=Start+Dir*L;
            Branch(WoodBeam,Start,End,8*S);
            Branch(SnowBranch,Start+FVector(0,0,9*S),End+FVector(0,0,9*S),12*S);
            for(int32 J=1;J<=3;++J)
            {
                const FVector Fork=FMath::Lerp(Start,End,J/3.f);
                const FVector Tip=Fork+FVector(Dir.X*80*S,Dir.Y*80*S,120*S);
                Branch(SnowBranch,Fork,Tip,9*S);
                Add(SnowBall,Tip,FVector(.22,.22,.75)*S,FRotator(0,Angle*180/PI,0));
                Add(SnowBall,Fork,FVector(.35,.35,.18)*S);
                for(int Side : {-1,1})
                {
                    const FVector Twig=Tip+FVector(-Dir.Y*Side*55,Dir.X*Side*55,65)*S;
                    Branch(SnowBranch,FMath::Lerp(Fork,Tip,.55f),Twig,4*S);
                }
            }
        }
    }
    Add(SnowBall,Base+FVector(20*S,0,H),FVector(.22,.22,.85)*S);
}

void ASkiSampleTrail::Pine(float X,float Y,float S)
{
    float GroundOffset=Y<-4000?120.f:15.f;
    if(GetActiveCourse()==2&&Y<=-1050&&Y>-4400)GroundOffset=450+FMath::Clamp((-Y-1050)/3350,0.f,1.f)*500;
    if(GetActiveCourse()==2&&Y<=-5800&&Y>=-7800)
    {
        const float Crest=180+260*FMath::Square(FMath::Sin(X*.00019f))+80*FMath::Sin(X*.00091f);
        GroundOffset=-FMath::Lerp(Crest*.45f,-120.f,(-Y-5800)/2000)-12;
    }
    const float Z=HeightAt(X)-GroundOffset-8;
    Branch(WoodBeam,FVector(X,Y,Z),FVector(X,Y,Z+600*S),15*S);
    for(int32 K=0;K<4;++K)
    {
        const float Radius=(3.9f-K*.8f)*S;
        Add(PineCone,FVector(X,Y,Z+(210+K*130)*S),FVector(Radius,Radius,3.1*S));
        Add(WhiteCone,FVector(X,Y,Z+(245+K*130)*S),FVector(Radius*.92,Radius*.92,2.7*S));
    }
}

void ASkiSampleTrail::Lodge(float X,float Y)
{
    const float Z=HeightAt(X)+15;
    Add(CedarCube,FVector(X,Y,Z+160),FVector(6.2,4.2,3.2));
    for(int I=0;I<10;++I) Add(WoodBeam,FVector(X,Y+215,Z+I*33),FVector(.34,.34,6.5),FRotator(90,0,0));
    // Pitched roof, snow thickness, exposed fascia and a warm front window.
    for(int Sign : {-1,1})
    {
        Add(DarkCube,FVector(X+Sign*175,Y,Z+395),FVector(4.2,5.6,.20),FRotator(-Sign*29,0,0));
        Add(SnowCube,FVector(X+Sign*175,Y,Z+416),FVector(4.35,5.7,.42),FRotator(-Sign*29,0,0));
    }
    Add(DarkCube,FVector(X+110,Y+219,Z+137),FVector(1.1,.13,2.4));
    Add(GlassCube,FVector(X-135,Y+223,Z+190),FVector(1.4,.08,1.35));
    Add(CedarCube,FVector(X-135,Y+229,Z+190),FVector(.08,.08,1.4));
    Add(CedarCube,FVector(X-135,Y+229,Z+190),FVector(1.4,.08,.08));
    Add(DarkCube,FVector(X+190,Y-70,Z+470),FVector(.65,.65,2.4));
    Add(SnowCube,FVector(X+190,Y-70,Z+595),FVector(.85,.85,.20));
    Add(SnowBall,FVector(X,Y+240,Z+10),FVector(8,2,.5));
}

void ASkiSampleTrail::Ground()
{
    const int32 Steps=FMath::CeilToInt((FinishX+9000)/50);
    TArray<FVector> V,N; TArray<int32> T; TArray<FVector2D> UV;
    const float Ys[]={-900,-600,-320,0,320,620,850};
    for(int I=0;I<=Steps;++I)
    {
        const float X=-3000+I*50;
        for(int J=0;J<7;++J)
        {
            const float Y=Ys[J];
            const float Shoulder=FMath::Abs(Y)>320 ? 18*FMath::Sin(X*.003+J)+FMath::Abs(Y)*.035f : 0;
            V.Add(FVector(X,Y,HeightAt(X)+Shoulder));
            N.Add(FVector(-(HeightAt(X+1)-HeightAt(X-1))*.5f,0,1).GetSafeNormal());
            UV.Add(FVector2D(X/500,Y/500));
            if(I<Steps && J<6)
            {
                const int A=I*7+J,B=A+7;
                T.Append({A,A+1,B,A+1,B+1,B});
            }
        }
    }
    Terrain->CreateMeshSection_LinearColor(0,V,T,N,UV,TArray<FLinearColor>(),TArray<FProcMeshTangent>(),true);
    // The far side blends into the riverbank rather than exposing a deep slab.
    V.Empty();N.Empty();UV.Empty();T.Empty();
    for(int32 I=0;I<=Steps;++I)
    {
        const float X=-3000+I*50;
        const float Shoulder=18*FMath::Sin(X*.003)+900*.035f;
        const float BankY[]={-900,-980,-1100};
        const float BankZ[]={Shoulder,-160,-450};
        for(int32 J=0;J<3;++J)
        {
            V.Add(FVector(X,BankY[J],HeightAt(X)+BankZ[J]));
            N.Add(FVector(0,-.8f,.6f));UV.Add(FVector2D(X/500,BankY[J]/500));
            if(I<Steps&&J<2){const int32 A=I*3+J,B=A+3;T.Append({A,B,A+1,A+1,B,B+1});}
        }
    }
    Terrain->CreateMeshSection_LinearColor(1,V,T,N,UV,TArray<FLinearColor>(),TArray<FProcMeshTangent>(),false);
    Terrain->SetMaterial(1,Terrain->GetMaterial(0));

    // Continue snow past the camera instead of dropping a vertical cut at Y=850.
    // The first row exactly shares the playable strip's edge; the outside edge
    // is far behind the furthest gameplay camera and cannot enter its view.
    V.Empty();N.Empty();UV.Empty();T.Empty();
    const float FrontY[]={850,1100,1600,2400,3600,5400,8000,12000,18000,26000};
    constexpr int32 FrontColumns=UE_ARRAY_COUNT(FrontY);
    auto SnowHeight=[](float X,float Y)
    {
        const float D=FMath::Max(0.f,Y-850);
        const float Edge=18*FMath::Sin(X*.003+6)+850*.035f;
        return HeightAt(X)+Edge*FMath::Exp(-D/1100)-.13f*D*D/(D+1800);
    };
    float MaxSeamGap=0;
    const FProcMeshSection* Playable=Terrain->GetProcMeshSection(0);
    for(int32 I=0;I<=Steps;++I)
    {
        const float X=-3000+I*50;
        for(int32 J=0;J<FrontColumns;++J)
        {
            const float Y=FrontY[J];const FVector P(X,Y,SnowHeight(X,Y));
            V.Add(P);UV.Add(FVector2D(X/500,Y/500));
            if(J==0)
            {
                N.Add(FVector(-(HeightAt(X+1)-HeightAt(X-1))*.5f,0,1).GetSafeNormal());
                MaxSeamGap=FMath::Max(MaxSeamGap,float(FVector::Dist(P,Playable->ProcVertexBuffer[I*7+6].Position)));
            }
            else N.Add(FVector(-(SnowHeight(X+1,Y)-SnowHeight(X-1,Y))*.5f,-(SnowHeight(X,Y+1)-SnowHeight(X,Y-1))*.5f,1).GetSafeNormal());
            if(I<Steps&&J<FrontColumns-1){const int32 A=I*FrontColumns+J,B=A+FrontColumns;T.Append({A,A+1,B,A+1,B+1,B});}
        }
    }
    Terrain->CreateMeshSection_LinearColor(5,V,T,N,UV,TArray<FLinearColor>(),TArray<FProcMeshTangent>(),false);
    Terrain->SetMaterial(5,Terrain->GetMaterial(0));
    UE_LOG(LogTemp,Display,TEXT("SKI_FOREGROUND seam_gap_cm=%.5f front_extent_y=26000 vertices=%d"),MaxSeamGap,V.Num());

}

void ASkiSampleTrail::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    SetActiveCourse(CourseIndex);
    // Rebuild the lookup after loading maps saved before additional scenery pools.
    Pools.Reset();
    for(int32 I=0;I<PoolCount;++I)
        Pools.Add(CastChecked<UHierarchicalInstancedStaticMeshComponent>(GetDefaultSubobjectByName(*FString::Printf(TEXT("Scenery%02d"),I))));
    for(auto* P:Pools) P->ClearInstances();
    Terrain->ClearMeshSection(6);
    Terrain->ClearMeshSection(7);
    Ground();
    if(CourseIndex>0){BuildChapterScenery();BuildCourseFurniture();return;}
    FRandomStream R(20270115);
    // Valley and distant bank follow the descent instead of staying on a flat plane.
    for(int Section=3;Section<=4;++Section)
    {
        TArray<FVector> V,N;TArray<int32> T;TArray<FVector2D> UV;
        const int32 Steps=FMath::CeilToInt((FinishX+9000)/100);
        for(int I=0;I<=Steps;++I)
        {
            const float X=-3000+I*100, Z=HeightAt(X)-(Section==3?450:120);
            for(int Side=0;Side<2;++Side)
            {
                V.Add(FVector(X,Section==3?(Side?-4400:-1050):(Side?-7800:-4400),Z));
                N.Add(FVector::UpVector);UV.Add(FVector2D(X/1800,Side));
            }
            if(I<Steps){int A=I*2;T.Append({A,A+2,A+1,A+1,A+2,A+3});}
        }
        Terrain->CreateMeshSection_LinearColor(Section,V,T,N,UV,TArray<FLinearColor>(),TArray<FProcMeshTangent>(),false);
        Terrain->SetMaterial(Section,Section==3?Pools[IceCube]->GetMaterial(0):Terrain->GetMaterial(0));
    }
    for(float X=-2000;X<FinishX+5000;X+=660)
    {
        const float TreeX=X+R.FRandRange(-160,160);
        const float TreeY=R.FRandRange(-720,-450),TreeScale=R.FRandRange(.68,1.12);
        // Keep nearby rime branches out of the finish portrait and fireworks sky.
        bool ClearView=FMath::Abs(TreeX-2500.f)<900.f || FMath::Abs(TreeX-(FinishX-8100.f))<900.f || FMath::Abs(TreeX-(FinishX-180.f))<2100.f;
        for(float Landmark : {900.f,1600.f,10100.f,19300.f,19500.f,40400.f,41000.f,45500.f,55000.f,68000.f,80500.f,95500.f,100400.f})
            ClearView|=FMath::Abs(TreeX-Landmark)<640;
        if(!ClearView)RimeTree(TreeX,TreeY,TreeScale,R);
        if(FMath::Fmod(X+2000,1320)<1) Pine(X+260,-4700,R.FRandRange(1.2,2.1));
    }
    RebuildBackdrop();
    // Preserve the scenery random stream used by the original mountain loop.
    for(float X=-6000;X<FinishX+13000;X+=2200)
    {
        R.FRandRange(1000,2100);
        Pine(X,-5700,R.FRandRange(1.4,2.1));
    }
    for(float X=-1500;X<FinishX+4000;X+=430)
    {
        Add(SnowBall,FVector(X+R.FRandRange(-70,70),R.FRandRange(490,700),HeightAt(X)-15),FVector(R.FRandRange(1.5,3.4),1.6,.60));
        if(R.FRand()<.24) Add(RockBall,FVector(X,620,HeightAt(X)-5),FVector(1.25,.95,.5));
    }
    Lodge(1600,-810); Lodge(19300,-850); Lodge(41000,-800);
    Lodge(67500,-950);Lodge(69100,-1150);Lodge(96200,-900);
    // Winter-fishing vignette: wooden drying racks, net lattice and fish.
    for(float X : {54800.f,55250.f,55700.f})
    {
        const float Z=HeightAt(X);
        for(float Dx : {-170.f,170.f})Branch(WoodBeam,FVector(X+Dx,-1150,Z),FVector(X+Dx,-1150,Z+390),10);
        Branch(WoodBeam,FVector(X-240,-1150,Z+340),FVector(X+240,-1150,Z+340),12);
        for(int I=0;I<6;++I)
        {
            Branch(GoldCube,FVector(X-170+I*68,-1140,Z+90),FVector(X-170+I*68,-1140,Z+310),2);
            Branch(WoodBeam,FVector(X-170,-1140,Z+100+I*36),FVector(X+170,-1140,Z+100+I*36),2);
        }
        Add(GlassCube,FVector(X,-1120,Z+230),FVector(.23,.2,.8),FRotator(18,0,0));
        Add(GoldCube,FVector(X+15,-1120,Z+178),FVector(.45,.15,.25),FRotator(45,0,0));
    }
    // Yanbian-inspired pavilion roof silhouette, set behind the playable lane.
    for(float X : {67400.f,69000.f})
    {
        const float Z=HeightAt(X);
        for(float Dx : {-320.f,320.f})Branch(FlagCube,FVector(X+Dx,-1750,Z),FVector(X+Dx,-1750,Z+510),18);
        for(int Side : {-1,1})
        {
            Add(DarkCube,FVector(X+Side*170,-1750,Z+560),FVector(4.8,7.5,.28),FRotator(-Side*17,0,0));
            Add(SnowCube,FVector(X+Side*380,-1750,Z+550),FVector(1.2,7.5,.16),FRotator(Side*20,0,0));
        }
        Add(GoldCube,FVector(X,-1350,Z+440),FVector(4,.15,.6));
    }
    // Changchun-inspired tram stop and warm-window street frontage.
    for(int I=0;I<5;++I)
    {
        const float X=79900+I*350,Z=HeightAt(X);
        Add(CedarCube,FVector(X,-1900,Z+460),FVector(3.2,5,9.2));
        Add(SnowCube,FVector(X,-1900,Z+930),FVector(3.5,5.4,.24));
        for(int J=0;J<3;++J)Add(GlassCube,FVector(X,-1630,Z+180+J*235),FVector(1.8,.06,1.1));
    }
    const float TramX=80900,TramZ=HeightAt(TramX);
    Add(FlagCube,FVector(TramX,-930,TramZ+185),FVector(11,3.3,3.3));
    Add(SnowCube,FVector(TramX,-930,TramZ+365),FVector(11.5,3.6,.2));
    for(int I=0;I<5;++I)Add(GlassCube,FVector(TramX-410+I*205,-756,TramZ+260),FVector(1.4,.08,1.4));
    for(float Dx : {-380.f,380.f})Add(RockBall,FVector(TramX+Dx,-770,TramZ+40),FVector(.8,.25,.8));
    BuildCourseFurniture();
}

void ASkiSampleTrail::BuildCourseFurniture()
{
    BuildRideableSurfaces();
    // Snow lanterns and trail lamps stay outside the collision lane.
    for(float X=2700;X<FinishX;X+=1800)
    {
        const float Z=HeightAt(X);
        Branch(DarkCube,FVector(X,-300,Z),FVector(X,-300,Z+470),9);
        Branch(GoldCube,FVector(X,-300,Z+470),FVector(X+100,-300,Z+470),6);
        Add(LampBall,FVector(X+100,-300,Z+418),FVector(.62,.62,.82));
        Add(DarkCube,FVector(X+100,-300,Z+465),FVector(.95,.95,.12));
        Add(SnowCube,FVector(X+100,-300,Z+480),FVector(1.04,1.04,.14));
    }
    for(float X=FinishX-8400;X<FinishX;X+=700)
    {
        const float Z=HeightAt(X);
        Branch(FlagCube,FVector(X,-650,Z),FVector(X,-650,Z+520),8);
        Add(LampBall,FVector(X,-650,Z+470),FVector(1.05,1.05,1.3));
        Add(FlagCube,FVector(X,-650,Z+390),FVector(.10,.10,.75));
    }
    // Finish / departure architectural frames: teal, brass and snow.
    for(float X : {1300.f,FinishX})
    {
        for(float Y : {-320.f,320.f})
        {
            Add(DarkCube,FVector(X,Y,HeightAt(X)+260),FVector(.30,.35,5.2));
            Add(GoldCube,FVector(X,Y,HeightAt(X)+120),FVector(.38,.43,.15));
        }
        Add(DarkCube,FVector(X,0,HeightAt(X)+530),FVector(.5,7.2,.48));
        Add(SnowCube,FVector(X,0,HeightAt(X)+558),FVector(.65,7.5,.15));
    }
    for(int Index=0;Index<ObstaclePositions().Num();++Index)
    {
        const float X=ObstaclePositions()[Index],H=ObstacleHeight(Index);
        if(GetActiveCourse()==1)
        {
            Add(Index%2?DarkCube:FlagCube,FVector(X,0,HeightAt(X)+H*.5f),FVector(1.5,3.4,H/100));
            for(float Y:{-105.f,0.f,105.f})Add(GoldCube,FVector(X+78,Y,HeightAt(X)+H*.55f),FVector(.05,.36,H*.006f));
        }
        else if(GetActiveCourse()==2&&Index%3==2)
        {
            Add(IceCube,FVector(X,0,HeightAt(X)+H*.47f),FVector(1.8,3.2,H/100),FRotator(0,8,0));
        }
        else if(Index%2)
        {
            Add(WoodBeam,FVector(X,0,HeightAt(X)+H*.48),FVector(H/100,H/100,3.5),FRotator(0,0,90));
        }
        else
        {
            Add(RockBall,FVector(X,0,HeightAt(X)+H*.45),FVector(2.0,2.4,H/60));
        }
        // Amber approach flags and a turquoise take-off stripe.
        for(float Y : {-280.f})
        {
            Branch(WoodBeam,FVector(X-1050,Y,HeightAt(X-1050)),FVector(X-1050,Y,HeightAt(X-1050)+210),4);
            Add(IsRideableObstacle(Index)?IceCube:FlagCube,FVector(X-1000,Y,HeightAt(X-1050)+177),FVector(.95,.06,.55));
        }
        Add(GoldCube,FVector(X-800,0,HeightAt(X-800)+3),FVector(.28,3.5,.055));
    }
    for(float X : RampPositions())
    {
        const float Pitch=FMath::RadiansToDegrees(FMath::Atan2(HeightAt(X+30)-HeightAt(X-30),60));
        for(float Dx : {-180.f,-90.f,0.f})
            Add(FlagCube,FVector(X+Dx,0,HeightAt(X+Dx)+5),FVector(.30,3.7,.06),FRotator(Pitch,0,0));
        for(float Y : {-300.f})
        {
            Branch(WoodBeam,FVector(X,Y,HeightAt(X)),FVector(X,Y,HeightAt(X)+280),5);
            Add(FlagCube,FVector(X+45,Y,HeightAt(X)+245),FVector(.9,.08,.65));
        }
    }
}

void ASkiSampleTrail::SetNightAmount(float Amount)
{
    if(!LampMaterial)LampMaterial=Pools[LampBall]->CreateDynamicMaterialInstance(0);
    if(LampMaterial)LampMaterial->SetScalarParameterValue(TEXT("NightGlow"),Amount*4);
}

// One indexed surface joins the far bank and every ridge. Adjacent sections
// share their boundary vertices, so large changes in track height cannot open
// gaps between separately positioned mountain blocks.
void ASkiSampleTrail::RebuildBackdrop()
{
    TArray<FVector> V,N;TArray<int32> T;TArray<FVector2D> UV;
    const float Ys[]={-7780,-8800,-10200,-11600,-13500,-17000};
    constexpr int32 Columns=6;
    const int32 Steps=FMath::CeilToInt((FinishX+20000)/100.f);
    for(int32 I=0;I<=Steps;++I)
    {
        const float X=-6000+I*100;
        const float Peak=GetActiveCourse()==1?1100+430*FMath::Sin(X*.00035f):GetActiveCourse()==2?2050+450*FMath::Sin(X*.00015f)+260*FMath::Sin(X*.00054f):1700+600*FMath::Sin(X*.00083f)+380*FMath::Sin(X*.00157f+.8f);
        const float RearPeak=GetActiveCourse()==1?1650+600*FMath::Sin(X*.00027f+1):GetActiveCourse()==2?3300+650*FMath::Sin(X*.00012f+1.8f):1900+680*FMath::Sin(X*.00065f+1.8f)+300*FMath::Sin(X*.0013f);
        const bool Valley=GetActiveCourse()==2;
        const float Heights[]={-120.f,Valley?-350.f:250.f,Valley?Peak-1800:Peak,Valley?(Peak-1800)*.55f:Peak*.58f,Valley?RearPeak-1900:RearPeak,-1200.f};
        for(int32 J=0;J<Columns;++J)
        {
            V.Add(FVector(X,Ys[J],HeightAt(X)+Heights[J]));
            N.Add(FVector::ZeroVector);UV.Add(FVector2D(X/1800,Ys[J]/1800));
            if(I<Steps&&J<Columns-1)
            {
                const int32 A=I*Columns+J,B=A+Columns;
                T.Append({A,B,A+1,A+1,B,B+1});
            }
        }
    }
    for(int32 I=0;I<T.Num();I+=3)
    {
        const int32 A=T[I],B=T[I+1],C=T[I+2];
        const FVector Normal=FVector::CrossProduct(V[C]-V[A],V[B]-V[A]);
        N[A]+=Normal;N[B]+=Normal;N[C]+=Normal;
    }
    for(FVector& Normal:N)Normal.Normalize();
    Terrain->CreateMeshSection_LinearColor(2,V,T,N,UV,TArray<FLinearColor>(),TArray<FProcMeshTangent>(),false);
    Terrain->SetMaterial(2,Terrain->GetMaterial(0));
    UE_LOG(LogTemp,Display,TEXT("SKI_BACKDROP connected_vertices=%d triangles=%d"),V.Num(),T.Num()/3);
}

void ASkiSampleTrail::RebuildCourse()
{
    OnConstruction(GetActorTransform());
}
