/*
**      $Id$
*/
/************************************************************************
*									*
*			     Copyright (C)  2002			*
*				Internet2				*
*			     All Rights Reserved			*
*									*
************************************************************************/
/*
**	File:		owamp.h
**
**	Author:		Jeff W. Boote
**			Anatoly Karp
**
**	Date:		Wed Mar 20 11:10:33  2002
**
**	Description:	
**	This header file describes the owamp API. The owamp API is intended
**	to provide a portable layer for implementing the owamp protocol.
*/
#ifndef	OWAMP_H
#define	OWAMP_H
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>

#define	OWP_MODE_UNDEFINED		(0)
#define	OWP_MODE_UNAUTHENTICATED	(01)
#define	OWP_MODE_AUTHENTICATED		(02)
#define	OWP_MODE_ENCRYPTED		(04)

/*
 * The 590 should eventually be replaced by a IANA blessed service name.
 */
#define OWP_CONTROL_SERVICE_NAME	"590"

/*
 * These structures are opaque to the API user.
 * They are used to maintain state internal to the library.
 */
typedef struct OWPContextRec	*OWPContext;
typedef struct OWPControlRec	*OWPControl;
typedef struct OWPAddrRec	*OWPAddr;

/* Codes for returning error severity and type. */
typedef enum {
	OWPErrFATAL=-4,
	OWPErrWARNING=-3,
	OWPErrINFO=-2,
	OWPErrDEBUG=-1,
	OWPErrOK=0
} OWPErrSeverity;

typedef enum {
	OWPErrPOLICY,
	OWPErrUNDEFINED
} OWPErrType;

typedef int		OWPBoolean;
typedef u_int32_t	OWPSID[4];
typedef u_int32_t	OWPSequence;
typedef char		OWPKID[8];
typedef u_int8_t	OWPKey[16];
typedef u_int32_t	OWPSessionModes;


typedef struct OWPTimeStampRec{
	u_int32_t		sec;
	u_int32_t		frac_sec;
	OWPBoolean		sync;
	u_int8_t		prec;
} OWPTimeStamp;

typedef enum {
	OWPUnspecifiedTest,	/* invalid value	*/
	OWPPoissonTest
} OWPTestType;

typedef struct{
	OWPTestType	test_type;
	OWPTimeStamp	start_time;
	u_int32_t	npackets;
	u_int32_t	typeP;
	u_int32_t	packet_size_padding;
	u_int32_t	InvLambda;
} OWPPoissonTestSpec;

typedef struct{
	OWPTestType	test_type;
	OWPTimeStamp	start_time;
	u_int32_t	npackets;
	u_int32_t	typeP;
	u_int32_t	packet_size_padding;
	/* make sure this is larger then any other TestSpec struct. */
	u_int32_t	padding[4];
} OWPTestSpec;

struct OWPEndpointRec{
	OWPBoolean	receiver;	/* true if endpoint recv */

	struct sockaddr	sa_addr;

	OWPSID		sid;
};

typedef struct OWPEndpointRec *OWPEndpoint;

/*
 * The following types are used to initialize the library.
 */
/*
 * This type is used to define the function that is called whenever an error
 * is encountered within the library.
 * This function should return 0 on success and non-0 on failure. If it returns
 * failure - the default library error function will be called.
 */
typedef int (*OWPErrFunc)(
	void		*app_data,
	OWPErrSeverity	severity,
	OWPErrType	etype,
	const char	*errmsg
);

/*
 * The value that is returned from this function will be passed
 * as app_data to the OWPCheckTestPolicyFunc,
 * EndpointInitFunc, EndpointInitHookFunc,
 * EndpointStartFunc and EndpointStopFunc.
 *
 *
 */
typedef void (*OWPCheckControlPolicyFunc)(
	void		*app_data,
	OWPSessionModes	mode_req,
	OWPKID		kid,
	struct sockaddr	*local_sa_addr,
	struct sockaddr	*remote_sa_addr,
	OWPErrSeverity	*err_ret
);

/*
 * This function will be called by OWPRequestTestSession if
 * one of the endpoints of the test is on the localhost before
 * it calls the EndpointInit*Func's. If err_ret returns
 * OWPErrFATAL, OWPRequestTestSession will not continue, and return
 * OWPErrFATAL as well.
 *
 * endpoint->sid will not be valid yet.
 * Only the IP address values will be set in the sockaddr structures -
 * i.e. port numbers will not be valid.
 */
typedef void (*OWPCheckTestPolicyFunc)(
	void		*app_data,
	OWPTestSpec	*test_spec,
	OWPEndpoint	local,
	OWPEndpoint	remote,
	OWPErrSeverity	*err_ret
);

/*
 * Allocate port and set it in the *endpoint->sa_addr structure.
 * (different for IPV4 and IPV6? May need to call getsockname?)
 * (How do we detect if user passed FD in - don't need to bind!)
 * If "recv" - also allocate and set endpoint->sid
 */
typedef void (*OWPEndpointInitFunc)(
	void		*app_data,
	OWPEndpoint	endpoint,
	OWPErrSeverity	*err_ret
);

/*
 * Given remote_addr/port (can "connect" to remote addr now)
 * return OK
 */
typedef void (*OWPEndpointInitHookFunc)(
	void		*app_data,
	OWPEndpoint	local_endpoint,
	OWPEndpoint	remote_endpoint,
	OWPErrSeverity	*err_ret
);

/*
 * Given start session
 */
typedef void (*OWPEndpointStart)(
	void		*app_data,
	OWPEndpoint	endpoint,
	OWPErrSeverity	*err_ret
);

/*
 * Given stop session
 */
typedef void (*OWPEndpointStop)(
	void		*app_data,
	OWPEndpoint	endpoint,
	OWPErrSeverity	*err_ret
);

/*
 * This type is used to define the function that is called to retrieve the
 * current timestamp.
 */
typedef OWPTimeStamp (*OWPGetTimeStampFunc)(
	void		*app_data,
	OWPErrSeverity	*err_ret
);

/*
 * This type is used to define the function that retrieves the shared
 * secret from whatever key-store is in use.
 */
typedef OWPKey	(*OWPGetKeyFunc)(
	void		*closure,
	OWPKID		kid,
	OWPErrSeverity	*err_ret
);


/* 
 * This structure encodes parameters needed to initialize the library.
 */ 
typedef struct {
	time_sec_perhaps		tm_out;
	void				*app_data;
	OWPErrFunc			*err_func;
	OWPGetKeyFunc			*get_aes_key;
	OWPCheckControlPolicyFunc	*check_control_func;
	OWPCheckTestPolicyFunc		*check_test_func;
	OWPEndpointInitFunc		*endpoint_init_func;
	OWPEndpointInitHookFunc		*endpoint_init_hook_func;
	OWPEndpointStart		*endpoint_start_func;
	OWPEndpointStop			*endpoint_stop_func;
	OWPGetTimeStampFunc		*get_timestamp_func;
} OWPInitializeConfigRec, *OWPInitializeConfig;


/*
 * API Functions
 *
 */
extern OWPContext
OWPContextInitialize(
	OWPInitializeConfig	config
);

extern void
OWPContextFree(
	OWPContext	ctx
);

/*
 * Error reporting routines - in the end these will just call the
 * function that is registered for the context as the OWPErrFunc
 */
extern void
OWPError(
	OWPContext	ctx,
	OWPErrSeverity	severity,
	OWPErrType	etype,
	const char	*fmt,
	...
);

#define OWPLine	__FILE__,__LINE__

extern void
OWPErrorLine(
	OWPContext	ctx,
	const char	*file,	/* fill with __FILE__ macro */
	int		line,	/* fill with __LINE__ macro */
	OWPErrSeverity	severity,
	OWPErrType	etype,
	const char	*fmt,
	...
);



/*
 * The OWPAddrBy* functions are used to allow the OWP API to more
 * adequately manage the memory associated with the many different ways
 * of specifying an address - and to provide a uniform way to specify an
 * address to the main API functions.
 * These functions return NULL on failure. (They call the error handler
 * to specify the reason.)
 */
extern OWPAddr
OWPAddrByNode(
	OWPContext	ctx,
	const char	*node	/* dns or valid char representation of addr */
);

extern OWPAddr
OWPAddrByAddrInfo(
	OWPContext		ctx,
	const struct addrinfo	*ai	/* valid addrinfo linked list	*/
);

extern OWPAddr
OWPAddrBySockFD(
	OWPContext	ctx,
	int		fd	/* fd must be an already connected socket */
);

extern OWPErrSeverity
OWPAddrFree(
	OWPAddr	addr
);

/*
 * OWPControlOpen allocates an OWPclient structure, opens a connection to
 * the OWP server and goes through the initialization phase of the
 * connection. This includes AES/CBC negotiation. It returns after receiving
 * the ServerOK message.
 *
 * This is typically only used by an OWP client application (or a server
 * when acting as a client of another OWP server).
 *
 * err_ret values:
 * 	OWPErrOK	completely successful - highest level mode ok'd
 * 	OWPErrINFO	session connected with less than highest level mode
 * 	OWPErrWARNING	session connected but future problems possible
 * 	OWPErrFATAL	function will return NULL - connection is closed.
 * 		(Errors will have been reported through the OWPErrFunc
 * 		in all cases.)
 * function return values:
 * 	If successful - even marginally - a valid OWPclient handle
 * 	is returned. If unsuccessful, NULL is returned.
 */
extern OWPControl
OWPControlOpen(
	OWPContext	ctx,
	OWPAddr		server_addr,
	int		mode_mask,	/* OR of OWPSessionModes */
	const OWPKID	*kid,		/* null if unwanted	*/
	const OWPKey	*key,		/* null if unwanted	*/
	OWPErrSeverity	*err_ret
);

/*
 * This function is used to configure the address specification
 * for either one of the sender or receiver endpoints prior to
 * requesting the server to configure that endpoint.
 */
extern OWPEndpoint
OWPServerConfigEndpoint(
	OWPAddr		addr,
	OWPErrSeverity	*err_ret
);

/*
 * This function is used to configure a reciever on this host
 */
extern OWPEndpoint
OWPCreateRecvEndpoint(
	OWPAddr		addr,
	OWPErrSeverity	*err_ret
);

/*
 * This function is used to configure a sender on this host
 */
extern OWPEndpoint
OWPCreateSendEndpoint(
	OWPAddr		addr,
	OWPErrSeverity	*err_ret
);

/*
 * Request a test session - if err_ret is OWPErrOK - then the function
 * returns a valid SID for the session.
 */
extern OWPSID
OWPRequestTestSession(
	OWPControl	control_handle,
	OWPEndpoint	sender,
	OWPEndpoint	receiver,
	OWPTestSpec	test_spec
	OWPErrSeverity	*err_ret
);

/*
 * Start all test sessions - if successful, err_ret is OWPErrOK.
 */
extern void
OWPStartTestSessions(
	OWPControl	control_handle,
	OWPErrSeverity	*err_ret
);

/*
 * If a send/recv endpoint is part of the local application, use
 * this function to start it after the OWPStartTestSessions function
 * returns successfully.
 */
extern void
OWPStartEndpoint(
	OWPEndpoint	send_or_recv,
	OWPErrSeverity	*err_ret
);

/*
 * Wait for test sessions to complete. This function will return the
 * following integer values:
 * 	<0	ErrorCondition (can cast to OWPErrSeverity)
 * 	0	StopSessions received (OWPErrOK)
 * 	1	wake_time reached
 * 	2	CollectSession received from other side, and this side has
 * 		a receiver endpoint.
 *	3	system event (signal)
 */
extern int
OWPWaitTestSessionStop(
	OWPControl	control_handle,
	OWPTimeStamp	wake_time,		/* abs time */
	OWPErrSeverity	*err_ret
);

/*
 * Return the file descriptor being used for the control connection. An
 * application can use this to call select or otherwise poll to determine
 * if anything is ready to be read but they should not read or write to
 * the descriptor.
 * This can be used in conjunction with the OWPWaitTestSessionStop
 * function so that the application can recieve user input, and only call
 * the OWPWaitTestSessionStop function when there is something to read
 * from the connection. (A timestamp in the past would be used in this case
 * so that OWPWaitTestSessionStop does not block.)
 *
 * If the control_handle is no longer connected - the function will return
 * a negative value.
 */
extern int
OWPGetControlFD(
	OWPControl	control_handle
);

/*
 * Send the StopSession message, and wait for the response.
 */
extern void
OWPSendStopSessions(
	OWPControl	control_handle,
	OWPErrSeverity	*err_ret
);

#endif	/* OWAMP_H */