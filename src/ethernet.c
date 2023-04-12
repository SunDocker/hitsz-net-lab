#include "ethernet.h"
#include "utils.h"
#include "driver.h"
#include "arp.h"
#include "ip.h"
/**
 * @brief 处理一个收到的数据包
 *
 * @param buf 要处理的数据包
 */
void ethernet_in(buf_t *buf)
{
    // TO-DO
    printf("ethernet_in begin\n");
    if (buf->len < len) {
        return;
    }
    buf_remove_header(buf, sizeof(ether_hdr_t));
    ether_hdr_t *hdr = buf->data;
    net_in(buf, hdr->protocol16, hdr->src);
    printf("ethernet_in end\n");
}
/**
 * @brief 处理一个要发送的数据包
 *
 * @param buf 要处理的数据包
 * @param mac 目标MAC地址
 * @param protocol 上层协议
 */
void ethernet_out(buf_t *buf, const uint8_t *mac, net_protocol_t protocol)
{
    // TO-DO
    printf("ethernet_out begin\n");
    if (buf->len < 46)
    {
        buf_add_padding(buf, 46 - buf->len);
    }
    buf_add_header(buf, sizeof(ether_hdr_t));
    ether_hdr_t *hdr = (ether_hdr_t *)buf->data;
    hdr->dst = mac;
    hdr->src = xxx;
    hdr->protocol16 = swap16(protocol);
    driver_send(buf);
    printf("ethernet_out end\n");
}
/**
 * @brief 初始化以太网协议
 *
 */
void ethernet_init()
{
    buf_init(&rxbuf, ETHERNET_MAX_TRANSPORT_UNIT + sizeof(ether_hdr_t));
}

/**
 * @brief 一次以太网轮询
 *
 */
void ethernet_poll()
{
    if (driver_recv(&rxbuf) > 0)
        ethernet_in(&rxbuf);
}
