#define DAN_LOG_LEVEL 1 //全局日志等级
#define DAN_POOL_KEEP_SIZE 17 //对象池的大小
#define DAN_POOL_MAX_SIZE 17*1024//对象池对象的上限
#define DAN_POOL_KEEP_TIME 300*1000*1000 //5分钟=300毫秒
#define DAN_MTU 1500 //以太网最大传输单元
#define DAN_TCP_MSS 1460 //TCP最大段大小
#define DAN_HEAP_SIZE 64*1024 //协程最大堆大小为64k
#define DAN_MAX_TASKS 1024*10 //epoll最大任务数
#define DAN_TCP_PORT 7773 //tcp 监听端口
#define DAN_MSG_ID 2 //消息ID
#define DAN_MSG_LEN 4 //消息长度
#define DAN_MSG_TICK 4 //消息处理间隔 毫秒
#define DAN_EPOLL_WAIT 4 //EPOLL阻塞时间 毫秒 
