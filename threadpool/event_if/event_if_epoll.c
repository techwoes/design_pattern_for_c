#include "tpool_event_if.h"
#include "dp_debug.h"
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <errno.h>

#define EVENT_EPOLL_DEFMAX (4096)
//loop to add event
#define EVENT_EPOLL_TIMEOUT (50)

struct event_epoll_t {
	int epfd;
	int maxevents;
	int curevent_cnt;
	int is_stop;
};

typedef struct event_epoll_t event_epoll_t, *EventEpoll;

struct event_epoll_handler_t {
	event_subscriber_t subscriber;
	void *arg;
};
typedef struct event_epoll_handler_t event_epoll_handler_t, *EventEpollHandler;

static inline int convert_etpoll_eveid2own(int eventflag) {
	int ret_eveflag=0;
	if(eventflag&EV_TPOOL_READ) ret_eveflag |= EPOLLIN;
	if(eventflag&EV_TPOOL_WRITE) ret_eveflag |= EPOLLOUT;
	if(eventflag&EV_TPOOL_HUNGUP) ret_eveflag |= EPOLLHUP;
	return ret_eveflag;
}

static inline short convert_etpoll_ownid2eve(int eventflag) {
	int ret_eveflag=0;
	if(eventflag&EPOLLIN) ret_eveflag |= EV_TPOOL_READ;
	if(eventflag&EPOLLOUT) ret_eveflag |= EV_TPOOL_WRITE;
	if(eventflag&EPOLLHUP) ret_eveflag |= EV_TPOOL_HUNGUP;
	return ret_eveflag;
}

/*! @name API for event if */
/*@{*/
/** event new */
EventInstance event_if_new(void) {
	//create main instance
	EventEpoll instance = calloc(1,sizeof(*instance));
	if(!instance) return NULL;

	//set member value
	instance->maxevents = EVENT_EPOLL_DEFMAX;
	instance->epfd = epoll_create(EVENT_EPOLL_DEFMAX);
	if(instance->epfd == -1) {
		DEBUG_ERRPRINT("Failed to open epoll!\n" );
		instance->epfd=0;
		goto err;
	}

	return instance;
err:
	event_if_free(instance);
	return NULL;
}

/** add new event */
EventHandler event_if_add(EventInstance this, EventSubscriber subscriber, void *arg) {

	EventEpoll base = (EventEpoll)this;
	/*max size reached*/
	if(base->maxevents <= base->curevent_cnt) {
		DEBUG_ERRPRINT("#######Oversize of event!\n");
		return NULL;
	}

	EventEpollHandler instance = calloc(1, sizeof(*instance));
	if(!instance) {
		DEBUG_ERRPRINT("#######calloc error!\n");
		return NULL;
	}

	//use subscriber for handler to get fd and delete event
	memcpy(&instance->subscriber, subscriber, sizeof(instance->subscriber));
	instance->arg = arg;

	struct epoll_event ev;
	ev.events = convert_etpoll_eveid2own(subscriber->eventflag);
	ev.data.ptr = instance;

	/*add event*/
	if(epoll_ctl(base->epfd, EPOLL_CTL_ADD, subscriber->fd, &ev) == -1) {
		DEBUG_ERRPRINT("Failed to new event! %s\n" , strerror(errno));
		goto err;
	}

	base->curevent_cnt++;

	return instance;
err:
	free(instance);
	return NULL;
}

/** update registered event */
void * event_if_update(EventInstance this, EventHandler handler, EventSubscriber subscriber, void *arg) {
	EventEpoll base = (EventEpoll)this;
	EventEpollHandler instance = (EventEpollHandler) handler;

	/*is different event?*/
	if(instance->subscriber.eventflag != subscriber->eventflag) {
		/*update event*/
		struct epoll_event event;
		event.events = convert_etpoll_eveid2own(subscriber->eventflag);
		event.data.ptr = handler;
		if(epoll_ctl(base->epfd, EPOLL_CTL_MOD, subscriber->fd, &event)==-1) {
			DEBUG_ERRPRINT("Failed to modify event! %s\n" , strerror(errno));
		}
	}

	/*copy subscribe*/
	memcpy(&instance->subscriber, subscriber, sizeof(instance->subscriber));
	instance->arg = arg;

	return handler;
}

/** delete event */
void event_if_del(EventInstance this, EventHandler handler) {
	EventEpoll base = (EventEpoll)this;
	EventEpollHandler instance = (EventEpollHandler) handler;
	if(epoll_ctl(base->epfd, EPOLL_CTL_DEL, instance->subscriber.fd, NULL)) {
		DEBUG_ERRPRINT("Failed to delete event! %s\n", strerror(errno) );
	}
	DEBUG_PRINT("free handle %p!\n", handler );
	free(handler);
	base->curevent_cnt--;
}

int event_if_getfd(EventHandler handler) {
	return ((EventEpollHandler)handler)->subscriber.fd;
}

/** main loop of this event */
int event_if_loop(EventInstance this) {
	EventEpoll base = (EventEpoll)this;
	int old_maxevents;
	int ret=0;
	struct epoll_event *events = malloc(base->maxevents * sizeof(struct epoll_event));
	if(!events) {
		return -1;
	}
	memset(events, 0, base->maxevents * sizeof(struct epoll_event));

	old_maxevents = base->maxevents;
	int cnt=0, i=0, loop=0;
	short eventflag;
	EventEpollHandler handler;
	base->is_stop=0;
	while(!base->is_stop) {
		loop=0;
		memset(events, 0, base->maxevents * sizeof(struct epoll_event));
		cnt = epoll_wait(base->epfd, events, base->maxevents, EVENT_EPOLL_TIMEOUT);
		if(cnt<0) {
			DEBUG_PRINT("Exit loop!\n" );
			ret = -1;
			break;
		}

		for(i=0;i<cnt;i++) {
			DEBUG_PRINT("check event [%d]\n" , i);
			if(!events[i].data.ptr) continue;
			eventflag = convert_etpoll_ownid2eve(events[i].events);
			handler = (EventEpollHandler)events[i].data.ptr;
			handler->subscriber.event_callback(handler->subscriber.fd, eventflag, handler->arg);
		}
	}
	DEBUG_PRINT("exit main\n" );

	free(events);
	return ret;
}

/** break event */
void event_if_loopbreak(EventInstance this) {
	EventEpoll base = (EventEpoll)this;
	base->is_stop=1;
}

/** exit after main loop */
void event_if_exit(EventInstance this) {
	return;
}

/** free event if instance */
void event_if_free(EventInstance this) {
	EventEpoll instance = (EventEpoll)this;
	if(!instance) return;
	if(instance->epfd) close(instance->epfd);
	free(instance);
}
/*@}*/
