#include "SkiSampleGameMode.h"
#include "SkiRunnerCharacter.h"
#include "SkiJourneyProgress.h"
#include "SkiSampleTrail.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "CanvasItem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
    const FLinearColor Cream(.95,.94,.86),Mint(.50,.87,.81),Dim(.52,.68,.70),Navy(.013,.039,.055,.96),Amber(.98,.65,.28);
    const TCHAR* Titles[]={TEXT("雾凇江岸"),TEXT("北大湖挑战"),TEXT("长白岩谷")};
    const TCHAR* Subtitles[]={TEXT("RIME / RIVERSIDE"),TEXT("BEIDAHU / ALPINE CLUB"),TEXT("CHANGBAI / SNOW VALLEY")};
}
void ASkiSampleHUD::Line(float X,float Y,float X2,float Y2,FLinearColor Color,float Thickness)
{
    const float S=Canvas->ClipY/900.f;FCanvasLineItem L(FVector2D(X*S,Y*S),FVector2D(X2*S,Y2*S));L.SetColor(Color);L.LineThickness=Thickness*S;Canvas->DrawItem(L);
}
void ASkiSampleHUD::Button(int32 Action,const FString& Label,const FString& Detail,float X,float Y,float W,float H,bool Active)
{
    const float S=Canvas->ClipY/900.f;float MX=-1,MY=-1;
    GetOwningPlayerController()->GetMousePosition(MX,MY);
    const bool Hover=MX>=X*S&&MX<=(X+W)*S&&MY>=Y*S&&MY<=(Y+H)*S;
    const bool Lit=Hover||Active;
    Panel(X+4,Y+5,W,H,FLinearColor(0,0,0,.22));
    Panel(X,Y,W,H,Lit?FLinearColor(.09,.25,.28,.98):FLinearColor(.035,.10,.13,.98));
    Panel(X,Y,3,H,Lit?Mint:FLinearColor(.17,.31,.34));
    Line(X+3,Y,X+W-15,Y,Lit?Mint:FLinearColor(.20,.34,.36));
    Line(X+W-15,Y,X+W,Y+15,Lit?Mint:Dim);
    Text(Label,X+23,Y+12,H<65?18:23,Lit?Cream:Dim);
    if(!Detail.IsEmpty())Text(Detail,X+24,Y+47,11,Lit?Mint:Dim);
    if(W>100)Text(TEXT("›"),X+W-33,Y+H*.5f-17,28,Lit?Mint:Dim);
    AddHitBox(FVector2D(X*S,Y*S),FVector2D(W*S,H*S),FName(*FString::FromInt(Action)),true);
}
void ASkiSampleHUD::NotifyHitBoxClick(FName Name)
{
    if(auto* P=Cast<ASkiRunnerCharacter>(GetOwningPawn()))P->MenuAction(FCString::Atoi(*Name.ToString()));
}
void ASkiSampleHUD::DrawSceneImage(int32 Index,float X,float Y,float W,float H,float Alpha,bool Moving)
{
    if(!MenuImages.IsValidIndex(Index)||!MenuImages[Index]||Alpha<.001f)return;
    auto* Texture=MenuImages[Index];const float S=Canvas->ClipY/900.f;
    const float Aspect=float(Texture->GetSizeX())/Texture->GetSizeY(),Target=W/H;
    const float T=GetWorld()->GetRealTimeSeconds();
    const float Zoom=Moving?1.055f+.009f*FMath::Sin(T*.19f+Index):1.f;
    const float U=FMath::Min(1.f,Target/Aspect)/Zoom,V=FMath::Min(1.f,Aspect/Target)/Zoom;
    const float CX=.5f+(Moving?.012f*FMath::Sin(T*.13f+Index*1.3f):0);
    const float CY=.5f+(Moving?.009f*FMath::Sin(T*.11f+Index):0);
    FCanvasTileItem Tile(FVector2D(X*S,Y*S),Texture->GetResource(),FVector2D(W*S,H*S),FLinearColor(1,1,1,Alpha));
    Tile.UV0=FVector2D(CX-U*.5f,CY-V*.5f);Tile.UV1=FVector2D(CX+U*.5f,CY+V*.5f);
    Tile.BlendMode=SE_BLEND_Translucent;Canvas->DrawItem(Tile);
}
void ASkiSampleHUD::DrawMenu(ASkiRunnerCharacter* P,float W)
{
    auto* G=Cast<USkiJourneyProgress>(GetGameInstance());
    if(MenuImages.IsEmpty())for(int32 I=1;I<=3;++I)MenuImages.Add(LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Ski25D/Textures/MenuScene%d.MenuScene%d"),I,I)));
    const int32 Selected=FMath::Clamp(P->SelectedCourse,0,2);
    if(MenuWeights[0]+MenuWeights[1]+MenuWeights[2]<.001f)MenuWeights[Selected]=1;
    const float Blend=1-FMath::Exp(-6.5f*FMath::Min(GetWorld()->GetDeltaSeconds(),.1f));
    for(int32 I=0;I<3;++I)MenuWeights[I]=FMath::Lerp(MenuWeights[I],I==Selected?1.f:0.f,Blend);
    Panel(0,0,W,900,FLinearColor::Black);
    float Sum=0;
    for(int32 I=0;I<3;++I)if(MenuWeights[I]>.0001f){Sum+=MenuWeights[I];DrawSceneImage(I,0,0,W,900,MenuWeights[I]/Sum,true);}
    Panel(0,0,W,900,FLinearColor(.005,.022,.034,P->MenuPage==2?.65f:.30f));
    for(int32 I=0;I<32;++I)Panel(I*W/32,0,W/32,900,FLinearColor(.005,.022,.034,.45f*(1-I/32.f)));
    // A travel-ticket frame, fine contours and punched-edge marks.
    for(int32 I=0;I<7;++I)Line(W-470+I*25,0,W-940+I*25,900,FLinearColor(.22,.55,.57,.08));
    Line(56,92,W-56,92,FLinearColor(.45,.69,.69,.35));
    Text(TEXT("吉雪奇旅"),58,34,28,Cream);Text(TEXT("JILIN  /  WINTER PASSPORT"),245,49,12,Mint);
    Text(TEXT("43° N   /   雪境探索计划"),W-295,48,12,Dim);
    Text(TEXT("欢迎相约吉林长春 · 长春2027第33届世界大学生冬季运动会"),58,112,18,Cream);
    Button(7,G&&G->bAudioMuted?TEXT("声音：关"):TEXT("声音：开"),TEXT(""),W-220,103,163,39);
    Text(TEXT("冬日有迹，热爱无界。"),58,826,15,Dim);
    Text(TEXT("↑ ↓ 选择   ENTER 确认   ← → 切景   ESC 返回   M 声音"),W-575,829,13,Dim);
    Text(TEXT("Music: Snowfall / Freedom — Scott Buckley · CC BY 4.0 · edited for game"),58,860,12,Dim);
    Text(TEXT("scottbuckley.com.au  |  creativecommons.org/licenses/by/4.0"),58,879,11,Dim);
    if(P->MenuPage==2)
    {
        Text(TEXT("旅程档案"),80,143,44,Cream);Text(TEXT("YOUR WINTER MEMORIES"),83,210,12,Mint);
        Text(G?FString::Printf(TEXT("已保存 %d 次旅程  /  最近 50 条"),G->History.Num()):TEXT("历史记录"),W-480,175,16,Dim);
        Panel(80,263,W-160,48,FLinearColor(.06,.16,.19,.94));
        Text(TEXT("日期 / 场景"),105,275,15,Mint);Text(TEXT("探索分数"),W*.49f,275,15,Mint);Text(TEXT("用时"),W*.62f,275,15,Mint);Text(TEXT("空翻"),W*.73f,275,15,Mint);Text(TEXT("结果"),W*.82f,275,15,Mint);
        if(!G||G->History.IsEmpty())
        {
            Text(TEXT("你的第一枚雪印，正在等待出发。"),W*.5f-265,405,27,Cream);
            Text(TEXT("完成或结束一次滑行后，会自动保存到这里。"),W*.5f-229,468,17,Dim);
        }
        else for(int32 Row=0;Row<6;++Row)
        {
            const int32 Index=P->HistoryPage*6+Row;if(!G->History.IsValidIndex(Index))break;
            const auto& E=G->History[Index];const float Y=321+Row*63;
            Panel(80,Y,W-160,59,FLinearColor(.025,.078,.10,.87));
            Text(E.Date,105,Y+5,12,Dim);Text(Titles[E.Chapter],105,Y+25,19,Cream);
            Text(FString::FromInt(E.Score),W*.49f,Y+17,20,Cream);
            Text(FString::Printf(TEXT("%02d:%02d"),int32(E.Seconds)/60,int32(E.Seconds)%60),W*.62f,Y+18,18,Dim);
            Text(FString::FromInt(E.Flips),W*.73f,Y+18,18,Dim);Text(E.Hearts>0?TEXT("成功抵达"):TEXT("途中惜败"),W*.82f,Y+18,16,E.Hearts>0?Mint:Amber);
        }
        Button(5,TEXT("返回首页"),TEXT(""),80,735,230,54);
        Button(3,TEXT("‹ 上一页"),TEXT(""),W-515,735,180,54);
        Button(4,TEXT("下一页 ›"),TEXT(""),W-285,735,180,54);
        Text(FString::Printf(TEXT("%d / %d"),P->HistoryPage+1,G?FMath::Max(1,(G->History.Num()+5)/6):1),W*.5f,751,15,Dim);
        if(G&&G->bSaveFailed)Text(TEXT("本次记录未写入磁盘，请检查磁盘空间。"),80,705,14,Amber);
        return;
    }
    const int32 C=P->SelectedCourse;
    Text(P->MenuPage==1?TEXT("下一站，去哪里？"):TEXT("沿着雪光"),80,177,52,Cream);
    Text(P->MenuPage==1?TEXT("三种风景 · 三种滑行节奏"):TEXT("遇见吉林。"),83,254,P->MenuPage==1?20:52,P->MenuPage==1?Dim:Cream);
    Text(TEXT("一双雪板，一段属于你的冬日旅行。"),84,349,17,Dim);
    const float BW=FMath::Min(420.f,W*.30f);
    Button(0,P->MenuPage==1?TEXT("出发 · 探索此景"):TEXT("开始游戏"),TEXT("START YOUR JOURNEY"),80,414,BW,83,P->MenuFocus==0);
    Button(1,TEXT("场景选择"),TEXT("THREE DESTINATIONS"),80,515,BW,83,P->MenuPage==1||P->MenuFocus==1);
    Button(2,TEXT("历史记录"),TEXT("YOUR WINTER MEMORIES"),80,616,BW,83,P->MenuFocus==2);
    if(P->MenuPage==1)Button(5,TEXT("返回首页"),TEXT(""),80,727,220,54);
    const float X=FMath::Max(560.f,W*.43f),CW=W-X-80;
    Panel(X+8,156,CW,552,FLinearColor(0,0,0,.28));Panel(X,148,CW,552,Navy);
    Sum=0;for(int32 I=0;I<3;++I)if(MenuWeights[I]>.0001f){Sum+=MenuWeights[I];DrawSceneImage(I,X,148,CW,330,MenuWeights[I]/Sum,false);}
    Panel(X+22,171,172,32,Navy);Text(FString::Printf(TEXT("目的地   %02d / 03"),C+1),X+35,177,14,Mint);
    for(int32 I=0;I<3;++I)Panel(X+CW-115+I*27,185,19,3,I==C?Mint:Dim);
    Text(Subtitles[C],X+28,499,12,Mint);Text(Titles[C],X+28,528,35,Cream);
    Text(C==0?TEXT("雾凇与江岸 · 长短交错的雪村下坡"):C==1?TEXT("缆车与飞坡 · 开阔明亮的竞技雪场"):TEXT("覆雪山脊与冰瀑 · 开阔的长白雪谷"),X+28,587,17,Dim);
    Line(X+28,632,X+CW-28,632,FLinearColor(.26,.47,.48));
    Text(G&&G->Completed[C]?FString::Printf(TEXT("最佳探索  %d"),G->ChapterScores[C]):TEXT("待探索 · 留下你的雪印"),X+28,654,15,Mint);
    Button(3,TEXT("‹"),TEXT(""),X+CW-151,643,52,45);
    Button(4,TEXT("›"),TEXT(""),X+CW-82,643,52,45);
}
