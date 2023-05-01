#include "net.h"
#include "ip.h"
#include "ethernet.h"
#include "arp.h"
#include "icmp.h"

/**
 * @brief 处理一个收到的数据包
 *
 * @param buf 要处理的数据包
 * @param src_mac 源mac地址
 */
void ip_in(buf_t *buf, uint8_t *src_mac)
{
    // TO-DO
    // header length: hdr_len or sizeof(ip_hdr_t) ?
    if (buf->len < sizeof(ip_hdr_t))
    {
        return;
    }
    ip_hdr_t *ip_hdr = (ip_hdr_t *)buf->data;
    if (ip_hdr->version != IP_VERSION_4)
    {
        return;
    }
    if (swap16(ip_hdr->total_len16) > buf->len)
    {
        return;
    }

    uint16_t checksum16_temp = ip_hdr->hdr_checksum16;
    ip_hdr->hdr_checksum16 = 0;
    if (checksum16(ip_hdr, sizeof(ip_hdr_t)) != checksum16_temp)
    {
        return;
    }
    ip_hdr->hdr_checksum16 = checksum16_temp;

    if (memcmp(net_if_ip, ip_hdr->dst_ip, NET_IP_LEN))
    {
        return;
    }

    if (buf->len > swap16(ip_hdr->total_len16))
    {
        buf_remove_padding(buf->len, buf->len - swap16(ip_hdr->total_len16));
    }

    buf_remove_header(buf, sizeof(ip_hdr_t));

    if (ip_hdr->protocol != NET_PROTOCOL_UDP)
    {
        icmp_unreachable(buf, ip_hdr->src_ip, ICMP_CODE_PROTOCOL_UNREACH);
        return;
    }

    // TODO IP or MAC
    net_in(buf, ip_hdr->protocol, ip_hdr->src_ip);
}

/**
 * @brief 处理一个要发送的ip分片
 *
 * @param buf 要发送的分片
 * @param ip 目标ip地址
 * @param protocol 上层协议
 * @param id 数据包id
 * @param offset 分片offset，必须被8整除
 * @param mf 分片mf标志，是否有下一个分片
 */
void ip_fragment_out(buf_t *buf, uint8_t *ip, net_protocol_t protocol, int id, uint16_t offset, int mf)
{
    // TO-DO
    buf_add_header(buf, sizeof(ip_hdr_t));
    ip_hdr_t *ip_hdr = (ip_hdr_t *)buf->data;

    ip_hdr->hdr_len = sizeof(ip_hdr_t) / IP_HDR_LEN_PER_BYTE;
    ip_hdr->version = IP_VERSION_4;
    ip_hdr->tos = 0;
    ip_hdr->total_len16 = swap16(buf->len);
    ip_hdr->id16 = swap16(id);
    uint16_t temp_flags_fragment16 = 0;
    temp_flags_fragment16 = offset / IP_HDR_OFFSET_PER_BYTE;
    if (mf)
    {
        temp_flags_fragment16 |= IP_MORE_FRAGMENT;
    }
    ip_hdr->flags_fragment16 = temp_flags_fragment16;
    ip_hdr->ttl = IP_DEFALUT_TTL;
    ip_hdr->protocol = protocol;
    ip_hdr->hdr_checksum16 = 0;
    memcpy(ip_hdr->src_ip, net_if_ip, NET_IP_LEN);
    memcpy(ip_hdr->dst_ip, ip, NET_IP_LEN);

    ip_hdr->hdr_checksum16 = checksum16(buf->data, sizeof(ip_hdr_t));

    arp_out(buf, ip);
}

/**
 * @brief 处理一个要发送的ip数据包
 *
 * @param buf 要处理的包
 * @param ip 目标ip地址
 * @param protocol 上层协议
 */
void ip_out(buf_t *buf, uint8_t *ip, net_protocol_t protocol)
{
    // TO-DO
    static int identifier = 0;

    uint16_t max_payload_len = ETHERNET_MAX_TRANSPORT_UNIT - sizeof(ip_hdr_t);
    uint16_t remained_payload_len = buf->len;

    if (remained_payload_len <= max_payload_len)
    {
        // TODO id?
        ip_fragment_out(buf, ip, protocol, identifier, 0, 0);
        return;
    }

    int idx;
    for (idx = 0; remained_payload_len > max_payload_len; idx++,
        remained_payload_len -= max_payload_len)
    {
        buf_t ip_buf;
        buf_init(&ip_buf, max_payload_len);
        memcpy(ip_buf.data, buf + idx * max_payload_len, max_payload_len);
        // TODO id?
        ip_fragment_out(&ip_buf, ip, protocol, identifier, idx * max_payload_len, 1);
    }
    buf_t ip_buf;
    buf_init(&ip_buf, remained_payload_len);
    memcpy(ip_buf.data, buf + idx * max_payload_len, remained_payload_len);
    // TODO id?
    ip_fragment_out(&ip_buf, ip, protocol, identifier, idx * max_payload_len, 0);

    identifier++;
}

/**
 * @brief 初始化ip协议
 *
 */
void ip_init()
{
    net_add_protocol(NET_PROTOCOL_IP, ip_in);
}