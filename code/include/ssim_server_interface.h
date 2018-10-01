/*!
 * @file	ssim_server_interface.h
 *
 * @author	skiller
 * @date	2018-10-01
 *
 * @brief	����ģ��ӿ�
 */

#include <cstdint>

#include <ssimdef.h>

namespace ssim
{

    // ���罻����ӿ�
    class NetworkInterface
    {
    public:
        // ��ʼ���˿ں͹����߳���
        virtual void Init(uint16_t port = 9301, int therad_num = 1) = 0;

        // ��ʼ����
        virtual void Run() = 0;

    };

    // ���ݷַ���ӿ�
    class MsgRouteInterface
    {
    public:
        virtual void PushMsgSendQueue() = 0;
        virtual void PopMsgSendQueue() = 0;

        virtual void PushMsgRecvQueue() = 0;
        virtual void PopMsgSendQueue() = 0;

        virtual void PushMsgPersistentQueue() = 0;
        virtual void PopMsgPersistentQueue() = 0;

        virtual void InsertSession() = 0;
        virtual void RemoveSession() = 0;
        virtual void IsActiveSession() = 0;
    };

    // ���ݴ����ӿ�
    class MsgProcessInterface
    {

    };

    // ���ݳ־ò�ӿ�
    class MsgPersistentInterface
    {

    };

}