/*
 Copyright (c) 2010, OpenEmu Team

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
     * Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
     * Neither the name of the OpenEmu Team nor the
       names of its contributors may be used to endorse or promote products
       derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY OpenEmu Team ''AS IS'' AND ANY
 EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL OpenEmu Team BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// We need to mess with core internals
#define M64P_CORE_PROTOTYPES 1

#import "MupenGameCore.h"
#import "api/config.h"
#import "api/m64p_common.h"
#import "api/m64p_config.h"
#import "api/m64p_frontend.h"
#import "api/m64p_vidext.h"
#import "api/callbacks.h"
#import "rom.h"
#import "savestates.h"
#import "osal/dynamiclib.h"
#import "version.h"
#import "memory.h"
#import "main.h"

#import <OpenEmuBase/OERingBuffer.h>
#import <OpenEmuBase/OETimingUtils.h>
#import <OpenGL/gl.h>

#import "plugin.h"

#import <dlfcn.h>

NSString *MupenControlNames[] = {
    @"N64_DPadU", @"N64_DPadD", @"N64_DPadL", @"N64_DPadR",
    @"N64_CU", @"N64_CD", @"N64_CL", @"N64_CR",
    @"N64_B", @"N64_A", @"N64_R", @"N64_L", @"N64_Z", @"N64_Start"
}; // FIXME: missing: joypad X, joypad Y, mempak switch, rumble switch

@interface MupenGameCore () <OEN64SystemResponderClient>
- (void)OE_didReceiveStateChangeForParamType:(m64p_core_param)paramType value:(int)newValue;
@end

__weak MupenGameCore *_current = 0;

static void (*ptr_OE_ForceUpdateWindowSize)(int width, int height);

static void MupenDebugCallback(void *context, int level, const char *message)
{
    if (level >= 5) return;
    NSLog(@"Mupen (%d): %s", level, message);
}

static void MupenStateCallback(void *context, m64p_core_param paramType, int newValue)
{
    NSLog(@"Mupen: param %d -> %d", paramType, newValue);
    [((__bridge MupenGameCore *)context) OE_didReceiveStateChangeForParamType:paramType value:newValue];
}

@implementation MupenGameCore
{
    NSData *romData;

    m64p_emu_state _emulatorState;

    dispatch_queue_t _callbackQueue;
    NSMutableDictionary *_callbackHandlers;

    BOOL _initializing;
    NSUInteger _frameCounter;

    //FBO obkects for OpenEmu and GlideN64 respectively
    GLint FBO, curFBO;

}

- (instancetype)init
{
    if (self = [super init]) {
        _initializing = YES;
        _frameCounter = 0;

        videoWidth  = 640;
        videoHeight = 480;
        videoBitDepth = 32; // ignored
        videoDepthBitDepth = 0; // TODO
        
        sampleRate = 33600;
        
        isNTSC = YES;

        _callbackQueue = dispatch_queue_create("org.openemu.MupenGameCore.CallbackHandlerQueue", DISPATCH_QUEUE_SERIAL);
        _callbackHandlers = [[NSMutableDictionary alloc] init];
    }
    _current = self;
    return self;
}

- (void)dealloc
{
    SetStateCallback(NULL, NULL);
    SetDebugCallback(NULL, NULL);
}

// Pass 0 as paramType to receive all state changes.
// Return YES from the block to keep watching the changes.
// Return NO to remove the block after the first received callback.
- (void)OE_addHandlerForType:(m64p_core_param)paramType usingBlock:(BOOL(^)(m64p_core_param paramType, int newValue))block
{
    // If we already have an emulator state, check if the block is satisfied with it or just add it to the queues.
    if(paramType == M64CORE_EMU_STATE && _emulatorState != 0 && !block(M64CORE_EMU_STATE, _emulatorState))
        return;

    dispatch_async(_callbackQueue, ^{
        NSMutableSet *callbacks = _callbackHandlers[@(paramType)];
        if(callbacks == nil)
        {
            callbacks = [[NSMutableSet alloc] init];
            _callbackHandlers[@(paramType)] = callbacks;
        }

        [callbacks addObject:block];
    });
}

- (void)OE_didReceiveStateChangeForParamType:(m64p_core_param)paramType value:(int)newValue
{
    if(paramType == M64CORE_EMU_STATE) _emulatorState = newValue;

    void(^runCallbacksForType)(m64p_core_param) =
    ^(m64p_core_param type){
        NSMutableSet *callbacks = _callbackHandlers[@(type)];
        [callbacks filterUsingPredicate:
         [NSPredicate predicateWithBlock:
          ^ BOOL (BOOL(^evaluatedObject)(m64p_core_param, int), NSDictionary *bindings)
          {
              return evaluatedObject(paramType, newValue);
          }]];
    };

    dispatch_async(_callbackQueue, ^{
        runCallbacksForType(paramType);
        runCallbacksForType(0);
    });
}

static void *dlopen_myself()
{
    Dl_info info;
    
    dladdr(dlopen_myself, &info);
    
    return dlopen(info.dli_fname, 0);
}

static void MupenGetKeys(int Control, BUTTONS *Keys)
{
    GET_CURRENT_OR_RETURN();

    Keys->R_DPAD = current->padData[Control][OEN64ButtonDPadRight];
    Keys->L_DPAD = current->padData[Control][OEN64ButtonDPadLeft];
    Keys->D_DPAD = current->padData[Control][OEN64ButtonDPadDown];
    Keys->U_DPAD = current->padData[Control][OEN64ButtonDPadUp];
    Keys->START_BUTTON = current->padData[Control][OEN64ButtonStart];
    Keys->Z_TRIG = current->padData[Control][OEN64ButtonZ];
    Keys->B_BUTTON = current->padData[Control][OEN64ButtonB];
    Keys->A_BUTTON = current->padData[Control][OEN64ButtonA];
    Keys->R_CBUTTON = current->padData[Control][OEN64ButtonCRight];
    Keys->L_CBUTTON = current->padData[Control][OEN64ButtonCLeft];
    Keys->D_CBUTTON = current->padData[Control][OEN64ButtonCDown];
    Keys->U_CBUTTON = current->padData[Control][OEN64ButtonCUp];
    Keys->R_TRIG = current->padData[Control][OEN64ButtonR];
    Keys->L_TRIG = current->padData[Control][OEN64ButtonL];
    Keys->X_AXIS = current->xAxis[Control];
    Keys->Y_AXIS = current->yAxis[Control];
}

static void MupenInitiateControllers (CONTROL_INFO ControlInfo)
{
    ControlInfo.Controls[0].Present = 1;
    ControlInfo.Controls[0].Plugin = PLUGIN_MEMPAK;
    ControlInfo.Controls[1].Present = 1;
    ControlInfo.Controls[1].Plugin = PLUGIN_MEMPAK;
    ControlInfo.Controls[2].Present = 1;
    ControlInfo.Controls[2].Plugin = PLUGIN_MEMPAK;
    ControlInfo.Controls[3].Present = 1;
    ControlInfo.Controls[3].Plugin = PLUGIN_NONE;
}

static AUDIO_INFO AudioInfo;

static void MupenAudioSampleRateChanged(int SystemType)
{
    GET_CURRENT_OR_RETURN();

    float currentRate = current->sampleRate;
    
    switch (SystemType)
    {
        default:
        case SYSTEM_NTSC:
            current->sampleRate = 48681812 / (*AudioInfo.AI_DACRATE_REG + 1);
            break;
        case SYSTEM_PAL:
            current->sampleRate = 49656530 / (*AudioInfo.AI_DACRATE_REG + 1);
            break;
    }

    [[current audioDelegate] audioSampleRateDidChange];
    NSLog(@"Mupen rate changed %f -> %f\n", currentRate, current->sampleRate);
}

static void MupenAudioLenChanged()
{
    GET_CURRENT_OR_RETURN();

    int LenReg = *AudioInfo.AI_LEN_REG;
    uint8_t *ptr = (uint8_t*)(AudioInfo.RDRAM + (*AudioInfo.AI_DRAM_ADDR_REG & 0xFFFFFF));

    // Swap channels
    for (uint32_t i = 0; i < LenReg; i += 4)
    {
        ptr[i] ^= ptr[i + 2];
        ptr[i + 2] ^= ptr[i];
        ptr[i] ^= ptr[i + 2];
        ptr[i + 1] ^= ptr[i + 3];
        ptr[i + 3] ^= ptr[i + 1];
        ptr[i + 1] ^= ptr[i + 3];
    }

    [[current ringBufferAtIndex:0] write:ptr maxLength:LenReg];
}

static void SetIsNTSC()
{
    GET_CURRENT_OR_RETURN();

    extern m64p_rom_header ROM_HEADER;
    switch (ROM_HEADER.Country_code&0xFF)
    {
        case 0x44:
        case 0x46:
        case 0x49:
        case 0x50:
        case 0x53:
        case 0x55:
        case 0x58:
        case 0x59:
            current->isNTSC = NO;
            break;
        case 0x37:
        case 0x41:
        case 0x45:
        case 0x4a:
            current->isNTSC = YES;
            break;
    }
}

static int MupenOpenAudio(AUDIO_INFO info)
{
    AudioInfo = info;
    
    SetIsNTSC();
    
    return M64ERR_SUCCESS;
}

static void MupenSetAudioSpeed(int percent)
{
    // do we need this?
}

- (BOOL)loadFileAtPath:(NSString *)path error:(NSError **)error
{
    // Load ROM
    romData = [NSData dataWithContentsOfFile:path options:NSDataReadingMappedIfSafe error:error];

    if (romData == nil) return NO;

    NSBundle *coreBundle = [NSBundle bundleForClass:[self class]];
    const char *dataPath;

    NSString *configPath = [[self supportDirectoryPath] stringByAppendingString:@"/"];
    dataPath = [[coreBundle resourcePath] fileSystemRepresentation];

    [[NSFileManager defaultManager] createDirectoryAtPath:configPath withIntermediateDirectories:YES attributes:nil error:nil];

    NSString *batterySavesDirectory = [self batterySavesDirectoryPath];
    [[NSFileManager defaultManager] createDirectoryAtPath:batterySavesDirectory withIntermediateDirectories:YES attributes:nil error:NULL];

    // open core here
    CoreStartup(FRONTEND_API_VERSION, [configPath fileSystemRepresentation], dataPath, (__bridge void *)self, MupenDebugCallback, (__bridge void *)self, MupenStateCallback);

    // set SRAM path
    m64p_handle config;
    ConfigOpenSection("Core", &config);
    ConfigSetParameter(config, "SaveSRAMPath", M64TYPE_STRING, [batterySavesDirectory UTF8String]);
    ConfigSetParameter(config, "SharedDataPath", M64TYPE_STRING, dataPath);
    ConfigSaveSection("Core");

    // Disable dynarec (for debugging)
    m64p_handle section;
//#ifdef DEBUG
//    int ival = 0;
//#else
    int ival = 2;
//#endif

    ConfigOpenSection("Core", &section);
    ConfigSetParameter(section, "R4300Emulator", M64TYPE_INT, &ival);

    if (CoreDoCommand(M64CMD_ROM_OPEN, (int)[romData length], (void *)[romData bytes]) != M64ERR_SUCCESS)
        return NO;

    return YES;
}

- (void)setupEmulation
{
    NSBundle *coreBundle = [NSBundle bundleForClass:[self class]];

    m64p_dynlib_handle core_handle = dlopen_myself();

    void (^LoadPlugin)(m64p_plugin_type, NSString *) = ^(m64p_plugin_type pluginType, NSString *pluginName){
        m64p_dynlib_handle rsp_handle;
        NSString *rspPath = [[coreBundle builtInPlugInsPath] stringByAppendingPathComponent:pluginName];

        rsp_handle = dlopen([rspPath fileSystemRepresentation], RTLD_NOW);
        ptr_PluginStartup rsp_start = osal_dynlib_getproc(rsp_handle, "PluginStartup");
        rsp_start(core_handle, (__bridge void *)self, MupenDebugCallback);
        
        CoreAttachPlugin(pluginType, rsp_handle);
    };

    // Load Video
    //LoadPlugin(M64PLUGIN_GFX, @"mupen64plus-video-rice.so");
    //LoadPlugin(M64PLUGIN_GFX, @"mupen64plus-video-glide64mk2.so");
    LoadPlugin(M64PLUGIN_GFX, @"mupen64plus-video-GLideN64.so");
    //LoadPlugin(M64PLUGIN_GFX, @"mupen64plus-video-angrylion-rdp-plus.so");

    ptr_OE_ForceUpdateWindowSize = dlsym(RTLD_DEFAULT, "_OE_ForceUpdateWindowSize");

    // Load Audio
    audio.aiDacrateChanged = MupenAudioSampleRateChanged;
    audio.aiLenChanged = MupenAudioLenChanged;
    audio.initiateAudio = MupenOpenAudio;
    audio.setSpeedFactor = MupenSetAudioSpeed;
    plugin_start(M64PLUGIN_AUDIO);

    // Load Input
    input.getKeys = MupenGetKeys;
    input.initiateControllers = MupenInitiateControllers;
    plugin_start(M64PLUGIN_INPUT);

    // Load RSP
    //LoadPlugin(M64PLUGIN_RSP, @"mupen64plus-rsp-hle.so");

    // Configure if using rsp-cxd4 plugin
    m64p_handle configRSP;
    ConfigOpenSection("rsp-cxd4", &configRSP);
    int usingHLE = 1; // Set to 0 if using LLE GPU plugin/software rasterizer such as Angry Lion
    ConfigSetParameter(configRSP, "DisplayListToGraphicsPlugin", M64TYPE_BOOL, &usingHLE);

    LoadPlugin(M64PLUGIN_RSP, @"mupen64plus-rsp-cxd4.so");
}

- (void)startEmulation
{
    [NSThread detachNewThreadSelector:@selector(runMupenEmuThread) toTarget:self withObject:nil];
    [super startEmulation];
}

- (void)runMupenEmuThread
{
    @autoreleasepool
    {
        OESetThreadRealtime(1. / 50, .007, .03); // guessed from bsnes
        [self.renderDelegate willRenderFrameOnAlternateThread];

        // Link the OpenEmu presentation buffer to the local FBO
        FBO = (GLint)[[self.renderDelegate presentationFramebuffer] integerValue];

        CoreDoCommand(M64CMD_EXECUTE, 0, NULL);
    }
}

- (void)videoInterrupt
{
    [self.renderDelegate didRenderFrameOnAlternateThread];
}

- (void)swapBuffers
{
    //Get the FBO attached to the Frmaebuffer
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &curFBO);

    //Bind GlideN64 FBO to read read and OpenEmu alternate FBO to draw buffer
    glBindFramebuffer(GL_READ_FRAMEBUFFER, curFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO);

    //Blit copy the GlideN64 FBO to the OpenEmu FBO
    //  flip and delete the 1 pixel of overscan while we are at it
    //  no more star trail left overs in Mario 64
    glBlitFramebuffer(0,  videoHeight, videoWidth - 1 , 0, 0, 0, videoWidth, videoHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    //Tell OpenEmu to Render the buffer
    //[self.renderDelegate presentDoubleBufferedFBO];
}

- (void)executeFrame
{
    // Do nothing
    if(_frameCounter >= 10)
        _initializing = NO;

    _frameCounter ++;
}

- (void)stopEmulation
{
    CoreDoCommand(M64CMD_STOP, 0, NULL);
    [super stopEmulation];
}

- (void)resetEmulation
{
    // FIXME: do we want/need soft reset? It doesn’t seem to work well with sending M64CMD_RESET alone
    // FIXME: might need to explicitly kick other thread
    CoreDoCommand(M64CMD_RESET, 1 /* hard reset */, NULL);
}

- (void)saveStateToFileAtPath:(NSString *)fileName completionHandler:(void (^)(BOOL, NSError *))block
{
    /*
     Blocks run in this order:
     scheduleSaveState -> M64CORE_STATE_SAVECOMPLETE
     */

    [self OE_addHandlerForType:M64CORE_STATE_SAVECOMPLETE usingBlock:
     ^ BOOL (m64p_core_param paramType, int newValue)
     {
         // Reset the paused state back to where it was.
         [self endPausedExecution];
         NSAssert(paramType == M64CORE_STATE_SAVECOMPLETE, @"This block should only be called for save completion!");
         dispatch_async(dispatch_get_main_queue(), ^{
             if(newValue == 0)
             {
                 NSError *error = [NSError errorWithDomain:OEGameCoreErrorDomain code:OEGameCoreCouldNotSaveStateError userInfo:@{
                     NSLocalizedDescriptionKey : @"Mupen Could not save the current state.",
                     NSFilePathErrorKey : fileName
                 }];
                 block(NO, error);
                 return;
             }

             block(YES, nil);
         });
         return NO;
     }];

    BOOL (^scheduleSaveState)(void) =
    ^ BOOL {
        if(CoreDoCommand(M64CMD_STATE_SAVE, 1, (void *)[fileName fileSystemRepresentation]) == M64ERR_SUCCESS)
        {
            // Mupen needs to be running to process the save.
            [self beginPausedExecution];
            return YES;
        }

        return NO;
    };

    if(scheduleSaveState()) return;

    [self OE_addHandlerForType:M64CORE_EMU_STATE usingBlock:
     ^ BOOL (m64p_core_param paramType, int newValue)
     {
         NSAssert(paramType == M64CORE_EMU_STATE, @"This block should only be called for load completion!");
         if(newValue != M64EMU_RUNNING && newValue != M64EMU_PAUSED)
             return YES;

         return !scheduleSaveState();
     }];
}

- (void)loadStateFromFileAtPath:(NSString *)fileName completionHandler:(void (^)(BOOL, NSError *))block
{
    if(_initializing)
    {
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1000 * NSEC_PER_MSEC), dispatch_get_main_queue(), ^{
            int success = CoreDoCommand(M64CMD_STATE_LOAD, 1, (void *)[fileName fileSystemRepresentation]);
            if(block) block(success==M64ERR_SUCCESS, nil);
       });
    }
    else
    {
        [self OE_addHandlerForType:M64CORE_STATE_LOADCOMPLETE usingBlock:
         ^ BOOL (m64p_core_param paramType, int newValue)
         {
             NSAssert(paramType == M64CORE_STATE_LOADCOMPLETE, @"This block should only be called for load completion!");

             [self endPausedExecution];
             dispatch_async(dispatch_get_main_queue(), ^{
                 if(newValue == 0)
                 {
                     NSError *error = [NSError errorWithDomain:OEGameCoreErrorDomain code:OEGameCoreCouldNotLoadStateError userInfo:@{
                         NSLocalizedDescriptionKey : @"Mupen Could not load the save state",
                         NSLocalizedRecoverySuggestionErrorKey : @"The loaded file is probably corrupted.",
                         NSFilePathErrorKey : fileName
                     }];
                     block(NO, error);
                     return;
                 }

                 block(YES, nil);
             });
             return NO;
         }];

        BOOL (^scheduleLoadState)(void) =
        ^ BOOL {
            if(CoreDoCommand(M64CMD_STATE_LOAD, 1, (void *)[fileName fileSystemRepresentation]) == M64ERR_SUCCESS)
            {
                // Mupen needs to be running to process the save.
                [self beginPausedExecution];
                return YES;
            }

            return NO;
        };

        if(scheduleLoadState()) return;

        [self OE_addHandlerForType:M64CORE_EMU_STATE usingBlock:
         ^ BOOL (m64p_core_param paramType, int newValue)
         {
             NSAssert(paramType == M64CORE_EMU_STATE, @"This block should only be called for load completion!");
             if(newValue != M64EMU_RUNNING && newValue != M64EMU_PAUSED)
                 return YES;

             return !scheduleLoadState();
         }];
    }
}

- (OEIntSize)bufferSize
{
    return OEIntSizeMake(videoWidth, videoHeight);
}

- (BOOL)tryToResizeVideoTo:(OEIntSize)size
{
    VidExt_SetVideoMode(size.width, size.height, 32, M64VIDEO_WINDOWED, 0);
    if (ptr_OE_ForceUpdateWindowSize) ptr_OE_ForceUpdateWindowSize(size.width, size.height);
    return YES;
}

- (OEGameCoreRendering)gameCoreRendering
{
    //return OEGameCoreRenderingOpenGL2Video;
    return OEGameCoreRenderingOpenGL3Video; // Set for GLideN64
}

- (BOOL)hasAlternateRenderingThread
{
    return YES;
}

//- (BOOL)needsDoubleBufferedFBO
//{
//    return YES;
//}

- (const void *)videoBuffer
{
    return NULL;
}

- (GLenum)pixelFormat
{
    return GL_BGRA;
}

- (GLenum)pixelType
{
    return GL_UNSIGNED_INT_8_8_8_8_REV;
}

- (GLenum)internalPixelFormat
{
    return GL_RGB8;
}

#pragma mark Mupen Audio

- (NSTimeInterval)frameInterval
{
    // Mupen uses 60 but it's probably wrong
    return isNTSC ? 60 : 50;
}

- (NSUInteger)channelCount
{
    return 2;
}

- (double)audioSampleRate
{
    return sampleRate;
}

- (oneway void)didMoveN64JoystickDirection:(OEN64Button)button withValue:(CGFloat)value forPlayer:(NSUInteger)player
{
    // N64 Programming Manual: The 3D Control Stick data is of type signed char and in the range between 80 and -80
    // TODO: handle analog gamepad deadzone and peak through API, e.g.
    /*
    int deadzone = 4096;
    int peak = 32767;
    int range = peak - deadzone;

    int joyVal = value * 32767;
    int axisVal = ((abs(joyVal) - deadzone) * 80 / range);
     */

    player -= 1;
    switch (button)
    {
        case OEN64AnalogUp:
            yAxis[player] = value * 80;
            break;
        case OEN64AnalogDown:
            yAxis[player] = value * -80;
            break;
        case OEN64AnalogLeft:
            xAxis[player] = value * -80;
            break;
        case OEN64AnalogRight:
            xAxis[player] = value * 80;
            break;
        default:
            break;
    }
}

- (oneway void)didPushN64Button:(OEN64Button)button forPlayer:(NSUInteger)player
{
    player -= 1;
    padData[player][button] = 1;
}

- (oneway void)didReleaseN64Button:(OEN64Button)button forPlayer:(NSUInteger)player
{
    player -= 1;
    padData[player][button] = 0;
}

- (void)setCheat:(NSString *)code setType:(NSString *)type setEnabled:(BOOL)enabled
{
    // Sanitize
    code = [code stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
    
    // Remove any spaces
    code = [code stringByReplacingOccurrencesOfString:@" " withString:@""];
    
    NSString *singleCode;
    NSArray *multipleCodes = [code componentsSeparatedByString:@"+"];
    m64p_cheat_code *gsCode = (m64p_cheat_code*) calloc([multipleCodes count], sizeof(m64p_cheat_code));
    int codeCounter = 0;
    
    for (singleCode in multipleCodes)
    {
        if ([singleCode length] == 12) // GameShark
        {
            // GameShark N64 format: XXXXXXXX YYYY
            NSString *address = [singleCode substringWithRange:NSMakeRange(0, 8)];
            NSString *value = [singleCode substringWithRange:NSMakeRange(8, 4)];
            
            // Convert GS hex to int
            unsigned int outAddress, outValue;
            NSScanner* scanAddress = [NSScanner scannerWithString:address];
            NSScanner* scanValue = [NSScanner scannerWithString:value];
            [scanAddress scanHexInt:&outAddress];
            [scanValue scanHexInt:&outValue];
            
            gsCode[codeCounter].address = outAddress;
            gsCode[codeCounter].value = outValue;
            codeCounter++;
        }
    }
    
    // Update address directly if code needs GS button pressed
    if ((gsCode[0].address & 0xFF000000) == 0x88000000 || (gsCode[0].address & 0xFF000000) == 0xA8000000)
    {
        *(unsigned char *)((g_rdram + ((gsCode[0].address & 0xFFFFFF)^S8))) = (unsigned char)gsCode[0].value; // Update 8-bit address
    }
    else if ((gsCode[0].address & 0xFF000000) == 0x89000000 || (gsCode[0].address & 0xFF000000) == 0xA9000000)
    {
        *(unsigned short *)((g_rdram + ((gsCode[0].address & 0xFFFFFF)^S16))) = (unsigned short)gsCode[0].value; // Update 16-bit address
    }
    // Else add code as normal
    else
    {
        enabled ? CoreAddCheat([code UTF8String], gsCode, codeCounter+1) : CoreCheatEnabled([code UTF8String], 0);
    }
}

@end
