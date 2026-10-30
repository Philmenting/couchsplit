// SPDX-License-Identifier: GPL-3.0-or-later
// Explicit development experiment, not a launcher mode. Uses the user's exact
// German FireRed and a private fresh-intro checkpoint; neither is distributed.
#include <mgba/core/core.h>
#include <mgba/core/config.h>
#include <mgba/core/log.h>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <array>
#include <stdexcept>

namespace {
constexpr auto RomHash="eed4fb0242bcbb6e0b0b80b197b47e3aec99ef4c5562cc45bae50ae86b970507";
constexpr uint32_t BufferA=0x02022BC4, Exec=0x02023BC8, Flags=0x02022B4C;
constexpr uint32_t Party=0x02024284, Enemy=0x0202402C, Moves=0x02023DC4;
void require(bool yes,const char* text){if(!yes)throw std::runtime_error(text);}
QByteArray digest(const QString& path){QFile f(path);require(f.open(QIODevice::ReadOnly),"Cannot read input");QCryptographicHash h(QCryptographicHash::Sha256);require(h.addData(&f),"Cannot hash input");return h.result().toHex();}

// Test-created level-10 parties, independent of all existing battery saves.
// Personality %24 ==0 gives the natural Growth/Attacks/EVs/Misc substructure order.
std::array<uint8_t,100> makeMon(unsigned species,unsigned personality){
    std::array<uint8_t,100> b{};
    auto u16=[&](unsigned at,unsigned n){b[at]=n;b[at+1]=n>>8;};
    auto u32=[&](unsigned at,unsigned n){for(unsigned k=0;k<4;k++)b[at+k]=n>>(8*k);};
    constexpr uint32_t ot=0xC0FFEE;
    u32(0,personality);u32(4,ot);b[8]=0xBB;b[9]=0xFF;b[0x12]=5;b[0x13]=2;b[0x14]=0xBB;b[0x15]=0xFF;
    u16(0x20,species);u32(0x24,1000);b[0x29]=70;
    u16(0x2C,33);u16(0x2E,45);b[0x34]=35;b[0x35]=40;
    u32(0x48,0x3FFFFFFF);
    unsigned sum=0;for(unsigned i=0x20;i<0x50;i+=2)sum+=b[i]|(b[i+1]<<8);u16(0x1C,sum);
    for(unsigned i=0x20;i<0x50;i+=4){uint32_t v=0;for(unsigned k=0;k<4;k++)v|=uint32_t(b[i+k])<<(k*8);u32(i,v^personality^ot);}
    b[0x54]=10;u16(0x56,50);u16(0x58,50);for(unsigned i=0x5A;i<0x64;i+=2)u16(i,25);
    return b;
}
struct Core {
    mCore* c=nullptr;bool initialized=false,configured=false;
    ~Core(){if(c){if(configured)mCoreConfigDeinit(&c->config);if(initialized)c->deinit(c);else free(c);}}
};
// Only this exact test ROM's ordinary doubles. This does not generalize to
// unrelated ROMs, native link battles, tutorials, safari, or simultaneous menus.
int inputOwner(mCore* c){
    const auto flags=c->busRead32(c,Flags);
    if(!(flags&1) || (flags&~0xDu))return -2;
    const auto pending=c->busRead32(c,Exec);int owner=-1;
    for(unsigned b:{0u,2u}){
        const auto cmd=c->busRead8(c,BufferA+b*0x200);
        if((pending&(1u<<b)) && (cmd==18 || cmd==20 || cmd==21 || cmd==22)){
            if(owner!=-1)return -2;
            owner=int(b/2);
        }
    }
    return owner;
}
}

int main(int argc,char** argv){
    QCoreApplication app(argc,argv);
    if(argc!=4){fprintf(stderr,"verify-firered-team-input ROM PRIVATE_INTRO_CHECKPOINT OUTPUT_DIRECTORY\n");return 2;}
    try{
        const QString rom=QString::fromLocal8Bit(argv[1]),state=QString::fromLocal8Bit(argv[2]),out=QString::fromLocal8Bit(argv[3]);
        require(digest(rom)==RomHash,"Only the exact German FireRed original is accepted");
        // mkdir is exclusive: never accept an existing output directory containing
        // saves, a checkpoint named result.json, or aliased screenshot paths.
        require(QDir().mkdir(out),"Output directory must be new, with an existing parent");
        mLogFilter filter{};mLogFilterInit(&filter);filter.defaultLevels=mLOG_FATAL|mLOG_ERROR;
        mLogger logger{};logger.filter=&filter;logger.log=[](mLogger*,int,mLogLevel,const char*,va_list){};mLogSetDefaultLogger(&logger);
        Core holder;holder.c=mCoreFind(rom.toUtf8().constData());require(holder.c,"Cannot find mGBA core");
        require(holder.c->init(holder.c),"Cannot initialize mGBA");holder.initialized=true;auto* c=holder.c;
        mCoreInitConfig(c,nullptr);holder.configured=true;mCoreOptions options{};options.useBios=false;options.skipBios=true;
        mCoreConfigLoadDefaults(&c->config,&options);mCoreLoadConfig(c);
        require(mCoreLoadFile(c,rom.toUtf8().constData()),"Cannot load ROM");
        std::array<color_t,240*160> video{};c->setVideoBuffer(c,video.data(),240);c->reset(c);
        QFile checkpoint(state);require(checkpoint.open(QIODevice::ReadOnly),"Cannot open private intro checkpoint");
        require(checkpoint.size()==qint64(c->stateSize(c)),"Unexpected checkpoint length");
        const auto bytes=checkpoint.readAll();require(bytes.size()==checkpoint.size(),"Cannot read full checkpoint");
        require(c->loadState(c,bytes.constData()),"Cannot restore private intro checkpoint");
        const auto save1=c->busRead32(c,0x03004F58),save2=c->busRead32(c,0x03004F5C);
        require(save1>=0x02000000 && save1<0x02040000 && save2>=0x02000000 && save2<0x02040000,"Intro checkpoint has no initialized game memory");
        auto run=[&](unsigned frames,unsigned keys){c->setKeys(c,keys);while(frames--)c->runFrame(c);};
        auto owned=[&](unsigned frames,unsigned player,unsigned keys){unsigned accepted=0;while(frames--){const bool own=inputOwner(c)==int(player);accepted+=own;c->setKeys(c,own?keys:0);c->runFrame(c);}return accepted;};
        auto capture=[&](const char* name){QImage image(240,160,QImage::Format_ARGB32);for(int y=0;y<160;y++)for(int x=0;x<240;x++){const auto p=video[y*240+x];image.setPixel(x,y,0xFF000000u|((p&255)<<16)|(p&0xFF00)|((p>>16)&255));}require(image.save(out+"/"+name+".png"),"Cannot save experiment capture");};
        for(unsigned i=0;i<600;i++){c->busWrite8(c,Party+i,0);c->busWrite8(c,Enemy+i,0);}
        const auto a=makeMon(1,24),b=makeMon(4,48),e=makeMon(19,72),f=makeMon(16,96);
        for(unsigned i=0;i<100;i++){c->busWrite8(c,Party+i,a[i]);c->busWrite8(c,Party+100+i,b[i]);c->busWrite8(c,Enemy+i,e[i]);c->busWrite8(c,Enemy+100+i,f[i]);}
        c->busWrite8(c,0x02024029,2);c->busWrite8(c,0x0202402A,2);
        c->busWrite32(c,Flags,1);c->busWrite32(c,0x03003044,0x0800FD21);
        run(600,0);run(5,1);run(200,0);run(5,1);run(200,0);
        require(inputOwner(c)==0,"Expected first player's action prompt");capture("player-one-action");
        const auto reject2=owned(10,1,1);run(40,0);
        require(reject2==0 && inputOwner(c)==0,"Player two changed player one's choice");
        owned(5,0,1);run(40,0);require(c->busRead8(c,BufferA)==20,"Player one did not reach move selection");
        owned(5,0,1);run(40,0);capture("player-one-move");
        owned(5,0,1);run(50,0);require(inputOwner(c)==1,"Expected second player's independent action prompt");
        const auto reject1=owned(10,0,1);run(30,0);
        require(reject1==0 && inputOwner(c)==1,"Player one changed player two's choice");
        capture("player-two-action");owned(5,1,1);run(40,0);owned(5,1,0x10);run(30,0);capture("player-two-move");
        owned(5,1,1);run(30,0);
        const auto firstMove=c->busRead16(c,Moves),secondMove=c->busRead16(c,Moves+4);
        require(firstMove==33 && secondMove==45,"The game did not accept distinct Tackle and Growl commands");
        // Let the actual engine execute the round. Never auto-answer a new choice.
        unsigned frames=0;while(frames<6000 && inputOwner(c)!=0){const auto owner=inputOwner(c);require(owner!=-2,"Ambiguous/unsupported battle state");run(1,owner<0 && frames%60<5?1:0);++frames;}
        require(inputOwner(c)==0,"The game did not return to a new turn");capture("after-first-round");
        require(digest(rom)==RomHash,"Original ROM changed");require(digest(state)==QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex(),"Private checkpoint changed");
        QJsonObject report{{"rom_sha256",RomHash},{"checkpoint_sha256",QString::fromLatin1(digest(state))},{"real_mgba_core",true},{"authored_test_parties",true},{"engine_accepts_distinct_moves",true},{"player_one_move",int(firstMove)},{"player_two_move",int(secondMove)},{"input_scope","single_core_test_harness"},{"test_harness_masks_wrong_owner",reject1==0 && reject2==0},{"round_completed",true},{"original_inputs_preserved",true},{"battery_save_files_opened",false},{"launcher_integration",false},{"two_worker_world_coop",false},{"save_result_transfer",false},{"physical_controllers",false},{"complete_firered_coop",false}};
        QFile result(out+"/result.json");require(result.open(QIODevice::WriteOnly|QIODevice::NewOnly),"Cannot create experiment report");const auto json=QJsonDocument(report).toJson();require(result.write(json)==json.size(),"Cannot write complete experiment report");result.close();
        printf("%s",json.constData());return 0;
    }catch(const std::exception& e){fprintf(stderr,"FireRed input experiment failed: %s\n",e.what());return 1;}
}
