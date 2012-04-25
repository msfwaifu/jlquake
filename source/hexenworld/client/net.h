// net.h -- quake's interface to the networking layer

extern netadr_t net_from;			// address of who sent the packet
extern QMsg net_message;

extern Cvar* hostname;

extern int net_socket;

void        NET_Init(int port);
void        NET_Shutdown(void);
qboolean    NET_GetPacket(void);
void        NET_SendPacket(int length, void* data, netadr_t to);

//============================================================================

#define OLD_AVG     0.99		// total = oldtotal*OLD_AVG + new*(1-OLD_AVG)

#define MAX_LATENT  32

void Netchan_Init(void);
void Netchan_Transmit(netchan_t* chan, int length, byte* data);
void Netchan_OutOfBand(netadr_t adr, int length, byte* data);
void Netchan_OutOfBandPrint(netadr_t adr, const char* format, ...);
qboolean Netchan_Process(netchan_t* chan);
void Netchan_Setup(netsrc_t sock, netchan_t* chan, netadr_t adr);

qboolean Netchan_CanPacket(netchan_t* chan);
qboolean Netchan_CanReliable(netchan_t* chan);
