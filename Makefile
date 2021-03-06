CC      = gcc
CXX     = g++
CXXFLAGS = -g -O3 -Wall -std=c++0x -pthread -I.
CFLAGS  = -g -O3 -Wall -pthread -I.
LIBS    = -lpthread
LDFLAGS = -g

OBJECTS = \
		edac/AMBEFEC.o \
		edac/BPTC19696.o \
		edac/CRC.o \
		edac/Golay2087.o \
		edac/Golay24128.o \
		edac/Hamming.o \
		edac/QR1676.o \
		edac/RS129.o \
		edac/RS634717.o \
		edac/SHA256.o \
		dmr/data/Data.o \
		dmr/data/EMB.o \
		dmr/data/EmbeddedData.o \
		dmr/lc/FullLC.o \
		dmr/lc/LC.o \
		dmr/lc/PrivacyLC.o \
		dmr/Transcode.o \
		dmr/Slot.o \
		dmr/SlotType.o \
		dmr/Sync.o \
		p25/data/LowSpeedData.o \
		p25/lc/LC.o \
		p25/Transcode.o \
		p25/P25Utils.o \
		network/UDPSocket.o \
		network/BaseNetwork.o \
		network/Network.o \
		vocoder/imbe/aux_sub.o \
		vocoder/imbe/basic_op.o \
		vocoder/imbe/ch_decode.o \
		vocoder/imbe/ch_encode.o \
		vocoder/imbe/dc_rmv.o \
		vocoder/imbe/decode.o \
		vocoder/imbe/dsp_sub.o \
		vocoder/imbe/encode.o \
		vocoder/imbe/imbe_vocoder.o \
		vocoder/imbe/math_sub.o \
		vocoder/imbe/pe_lpf.o \
		vocoder/imbe/pitch_est.o \
		vocoder/imbe/pitch_ref.o \
		vocoder/imbe/qnt_sub.o \
		vocoder/imbe/rand_gen.o \
		vocoder/imbe/sa_decode.o \
		vocoder/imbe/sa_encode.o \
		vocoder/imbe/sa_enh.o \
		vocoder/imbe/tbls.o \
		vocoder/imbe/uv_synt.o \
		vocoder/imbe/v_synt.o \
		vocoder/imbe/v_uv_det.o \
		vocoder/ambe3600x2250.o \
		vocoder/ambe3600x2400.o \
		vocoder/ambe3600x2450.o \
		vocoder/ecc.o \
		vocoder/imbe7200x4400.o \
		vocoder/mbe.o \
		vocoder/MBEDecoder.o \
		vocoder/MBEEncoder.o \
		yaml/Yaml.o \
		host/Host.o \
		Log.o \
		Mutex.o \
		Thread.o \
		Timer.o \
		StopWatch.o \
		Utils.o \
		HostMain.o

all:	dvmtranscode

dvmtranscode: $(OBJECTS) 
		$(CXX) $(OBJECTS) $(CXXFLAGS) $(LIBS) -o dvmtranscode

%.o: %.cpp
		$(CXX) $(CXXFLAGS) -c -o $@ $<
%.o: %.c
		$(CC) $(CFLAGS) -c -o $@ $<

clean:
		$(RM) dvmtranscode *.o *.d *.bak *~ edac/*.o dmr/*.o dmr/data/*.o dmr/lc/*.o p25/*.o p25/data/*.o p25/lc/*.o network/*.o vocoder/imbe/*.o vocoder/*.o yaml/*.o host/*.o
