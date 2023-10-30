#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <AL/al.h>
#include <AL/alc.h>

unsigned int    sample_rate = 44100;
ALCdevice       *openal_output_device;
ALCcontext      *openal_output_context;
ALuint          source;

int al_check_error(const char *given_label) {
    ALenum al_error;
    al_error = alGetError();
    if (AL_NO_ERROR != al_error) {
        printf("ERROR - %s  (%s)\n", alGetString(al_error), given_label);
        return al_error;
    }
    return 0;
}

void MM_init_al() {
    const char *defname = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
    printf("MM_init_al: defname %s\n", defname);

    openal_output_device = alcOpenDevice(defname);
    openal_output_context = alcCreateContext(openal_output_device, NULL);
    alcMakeContextCurrent(openal_output_context);

    // setup buffer and source
    al_check_error("failed call to alGenBuffers");
}

void MM_exit_al() {
    // Stop the sources
    alSourceStopv(1, &source); //      streaming_source
    alSourcei(source, AL_BUFFER, 0); // зачем?
    // Clean-up
    alDeleteSources(1, &source);
    alDeleteBuffers(1, &source);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(openal_output_context);
    alcCloseDevice(openal_output_device);
}

static void state_print(ALenum state) {
    switch (state) {
        case AL_INITIAL:
            printf("AL_INITIAL\n");
        break;
        case AL_PLAYING:
            printf("AL_PLAYING\n");
        break;
        case AL_PAUSED:
            printf("AL_PAUSED\n");
        break;
        case AL_STOPPED:
            printf("AL_STOPPED\n");
        break;
    }
}

void play_buffer(short *buff, size_t buff_size) {
    // reset, allow buffer changes
    alGenSources(1, &source);

    alSourcef(source, AL_PITCH, 1);
    // check for errors
    alSourcef(source, AL_GAIN, 1);
    // check for errors
    alSource3f(source, AL_POSITION, 0, 0, 0);
    // check for errors
    alSource3f(source, AL_VELOCITY, 0, 0, 0);
    // check for errors
    alSourcei(source, AL_LOOPING, AL_FALSE);

    ALuint buffer;
    alGenBuffers(1, &buffer);
    al_check_error("gen buffer");

    alBufferData(buffer, AL_FORMAT_MONO16, buff, buff_size, sample_rate);
    al_check_error("buffer data");
    alSourcei(source, AL_BUFFER, buffer);
    al_check_error("source buffer");

    alSourcePlay(source);

    ALenum current_playing_state;
    alGetSourcei(source, AL_SOURCE_STATE, &current_playing_state);
    al_check_error("alGetSourcei AL_SOURCE_STATE");

    state_print(current_playing_state);

    while (AL_PLAYING == current_playing_state) {
        //state_print(current_playing_state);
        //printf("still playing ... so sleep\n");

        // sleep(1);   // should use a thread sleep NOT sleep() for a more responsive finish

        alGetSourcei(source, AL_SOURCE_STATE, &current_playing_state);
        al_check_error("alGetSourcei AL_SOURCE_STATE");
    }
}

static void list_audio_devices(const ALCchar *devices) {
        const ALCchar *device = devices, *next = devices + 1;
        size_t len;

        fprintf(stdout, "Devices list:\n");
        fprintf(stdout, "----------\n");
        while (device && *device != '\0' && next && *next != '\0') {
                fprintf(stdout, "%s\n", device);
                len = strlen(device);
                device += (len + 1);
                next += (len + 2);
        }
        fprintf(stdout, "----------\n");
}

struct GeneratorSin {
    double  amplitude;
    double  duration;   // in seconds
};

struct GeneratorSquare {
    double  amplitude;
    double  duration;   // in seconds
};

typedef short int (*Generator)(double t, void *ctx);

// 0 <= t <= duration
static short int gen_sin(double t, void *ctx) {
    assert(ctx);
    struct GeneratorSin *g = ctx;
    return sinf(t) * g->amplitude;
}

static short int gen_square(double t, void *ctx) {
    assert(ctx);
    struct GeneratorSquare *g = ctx;
    return sinf(t) * g->amplitude;
}

static void MM_render_one_buffer() {
    // double freq = 440.f;

    // Время измеряется в дробных частях секунды
    double seconds = 3.;

    // количество кусочков звука
    size_t buf_len = floor(seconds) * sample_rate; 

    size_t buf_size = buf_len * sizeof(short int);
    short int *data = malloc(buf_size);
    assert(data);
    memset(data, 0, buf_size);

    srand(time(NULL));

    struct GeneratorSin gs = {
        .amplitude = 10000.,
        .duration = .1,
    };

    Generator gen = gen_sin;

    // Как записать треугольник?
    // Как записать квадрат?
    // Интерфейс записи волны

    double duration = 0.;
    //double dt = seconds / sample_rate;
    double dt = 1. / sample_rate;
    printf("dt %f\n", dt);
    for (int i = 0; i < buf_len; i += 2) {
        data[i] += gen(duration, &gs);
        printf("data[%d] = %d\n", i, data[i]);
        printf("duration %f\n", duration);
        duration += dt;
        if (duration >= gs.duration) {
            printf("duration reset\n");
            duration = 0.;
        }
    }

    /*
    for (int i = 0; i < buf_len / 2; i += 2) {
        data[i] += sin(i + M_PI * 2) * 2000.;
    }

    for (int i = buf_len / 2; i < buf_len; i += 2) {
        data[i] = rand() % 1000;
    }
    */

    play_buffer(data, buf_size);

    free(data);
    data = NULL;
}


int main() {
    list_audio_devices(alcGetString(NULL, ALC_DEVICE_SPECIFIER));

    MM_init_al();
    MM_render_one_buffer();
    MM_exit_al();

    return 0;
}
