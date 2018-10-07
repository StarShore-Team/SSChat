﻿#include "msg_process.h"

#include <package.hpp>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

using namespace ssim;

void ssim::msg_process::init(std::shared_ptr<msg_route_interface> msg_route,
    std::shared_ptr<msg_persistent_interface> msg_persistent,
    int thread_num)
{
    msg_route_ = msg_route;
    msg_persistent_ = msg_persistent;
    thread_num_ = thread_num;
}

void msg_process::run()
{
    t_persistent_ = std::thread(&msg_process::do_persistent, this);
    t_work_ = std::thread(&msg_process::do_work, this);
}

void ssim::msg_process::do_persistent()
{
    for (;;) {
        // TODO: 安全停止机制，程序停止时保证队列数据消耗完毕

        auto p_data = msg_route_->pop_msg_persistent_queue();
        auto user = unpack_dis_user(*p_data);
        if (user.empty()) {
            // TODO: log，正常逻辑不应当为空，所有持久化队列数据必然是某个消息
            continue;
        }

        msg_persistent_->persist_msg(user.c_str(), p_data);
    }
}

void ssim::msg_process::do_work()
{
    for (;;) {
        // TODO: 安全停止机制，程序停止时保证队列数据消耗完毕

        auto data = msg_route_->pop_msg_recv_queue();
        SSIM_header ssim_header;

        size_t nPtr = 0;

        unpack(*data.p_data_, nPtr, ssim_header.subtype_);
        nPtr += sizeof(ssim_header.subtype_);
        ssim_header.subtype_ = ntohl(ssim_header.subtype_);

        unpack(*data.p_data_, nPtr, ssim_header.errno_);
        nPtr += sizeof(ssim_header.errno_);
        ssim_header.errno_ = ntohl(ssim_header.errno_);

        unpack(*data.p_data_, nPtr, ssim_header.refid_);
        nPtr += sizeof(ssim_header.refid_);
        ssim_header.refid_ = ntohll(ssim_header.refid_);

        std::shared_ptr<std::vector<uint8_t>> ret;

        switch (ssim_header.subtype_) {
        case 0x01:
            ret = analysis_login(ssim_header, *data.p_data_, data.session_id_);
            break;
        case 0x02:
            ret = analysis_msg(ssim_header, *data.p_data_, data.session_id_);
            break;
        case 0x03:
            ret = analysis_regist(ssim_header, *data.p_data_, data.session_id_);
            break;
        case 0x04:
            ret = analysis_passwd_x4(ssim_header, *data.p_data_, data.session_id_);
            break;
        case 0x05:
            ret = analysis_passwd_x5(ssim_header, *data.p_data_, data.session_id_);
            break;
        case 0x06:
            ret = analysis_logout(ssim_header, *data.p_data_, data.session_id_);
            break;
        default:
            ret = analysis_no_support(ssim_header, *data.p_data_, data.session_id_);
            break;;
        }

        session_data ret_data = { data.session_id_, ret };
        msg_route_->push_msg_send_queue(ret_data);
    }
}

std::shared_ptr<std::vector<uint8_t>> ssim::msg_process::analysis_login(SSIM_header& header, std::vector<uint8_t>& data, uint64_t sessid)
{
    size_t nPtr = sizeof(header);
    auto user = unpack_string(data, nPtr, 16);
    nPtr += 16;
    auto pass = unpack_string(data, nPtr, 32);

    auto ret = msg_persistent_->check_user_passwd(user.c_str(), pass.c_str());

    active_user user_map{};

    if (0 == ret) {
        // 校验通过，保留会话id
        user_map.refid_ = create_refid();
        user_map.sessid_ = sessid;

        insert_active_user(user, user_map);
    }
}

std::shared_ptr<std::vector<uint8_t>> ssim::msg_process::analysis_msg(SSIM_header& header, std::vector<uint8_t>& data, uint64_t sessid)
{

}

std::shared_ptr<std::vector<uint8_t>> ssim::msg_process::analysis_regist(SSIM_header& header, std::vector<uint8_t>& data, uint64_t sessid)
{

}

std::shared_ptr<std::vector<uint8_t>> ssim::msg_process::analysis_passwd_x4(SSIM_header& header, std::vector<uint8_t>& data, uint64_t sessid)
{

}

std::shared_ptr<std::vector<uint8_t>> ssim::msg_process::analysis_passwd_x5(SSIM_header& header, std::vector<uint8_t>& data, uint64_t sessid)
{

}

std::shared_ptr<std::vector<uint8_t>> ssim::msg_process::analysis_logout(SSIM_header & header, std::vector<uint8_t>& data, uint64_t sessid)
{
    return std::shared_ptr<std::vector<uint8_t>>();
}

std::shared_ptr<std::vector<uint8_t>> ssim::msg_process::analysis_no_support(SSIM_header& header, std::vector<uint8_t>& data, uint64_t sessid)
{

}

std::shared_ptr<std::vector<uint8_t>> ssim::msg_process::create_feedback(uint32_t subtype, uint32_t ss_err, uint64_t refid)
{
    auto p_data = std::make_shared<std::vector<uint8_t>>();

    subtype = htonl(subtype);
    ss_err = htonl(ss_err);
    refid = htonll(refid);

    pack(*p_data, p_data->size(), subtype);
    pack(*p_data, p_data->size(), ss_err);
    pack(*p_data, p_data->size(), refid);

    return p_data;
}

uint64_t ssim::msg_process::create_refid()
{
    return uint64_t();
}

std::string ssim::msg_process::unpack_dis_user(std::vector<uint8_t>& data)
{
    uint32_t subtype;
    unpack(data, 0, subtype);
    subtype = ntohl(subtype);

    if (subtype != 0x00000002 || data.size() < 48) {
        return std::string();
    }

    char user[16];
    memcpy(user, &data[32], sizeof(user));
    return std::string(user);
}

void ssim::msg_process::insert_active_user(const std::string & user, active_user user_map)
{
    // TODO: 1-添加到在线用户表； 2-查询当前用户离线消息并将离线消息发送
    {
        std::shared_lock<std::shared_mutex> lk(users_mu_);
    }
}

void ssim::msg_process::remove_active_user(const std::string & user)
{
}

SSIM_API std::shared_ptr<msg_process_interface> ssim::create_msg_process_interface()
{
    return std::shared_ptr<msg_process>();
}
