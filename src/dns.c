/*************************************************************************
 *
 * Copyright (C) 2018 Ruilin Peng (Nick) <pymumu@gmail.com>.
 *
 * smartdns is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * smartdns is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE
#include "dns.h"
#include "tlog.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define QR_MASK 0x8000
#define OPCODE_MASK 0x7800
#define AA_MASK 0x0400
#define TC_MASK 0x0200
#define RD_MASK 0x0100
#define RA_MASK 0x0080
#define RCODE_MASK 0x000F
#define DNS_RR_END (0XFFFF)

/* read short and move pointer */
short dns_read_short(unsigned char **buffer)
{
	unsigned short value;

	value = ntohs(*((unsigned short *)(*buffer)));
	*buffer += 2;
	return value;
}

/* write char and move pointer */
void dns_write_char(unsigned char **buffer, unsigned char value)
{
	**buffer = value;
	*buffer += 1;
}

/* read char and move pointer */
unsigned char dns_read_char(unsigned char **buffer)
{
	unsigned char value = **buffer;
	*buffer += 1;
	return value;
}

/* write short and move pointer */
void dns_write_short(unsigned char **buffer, unsigned short value)
{
	value = htons(value);
	*((unsigned short *)(*buffer)) = value;
	*buffer += 2;
}

/* write int and move pointer */
void dns_write_int(unsigned char **buffer, unsigned int value)
{
	value = htonl(value);
	*((unsigned int *)(*buffer)) = value;
	*buffer += 4;
}

/* read int and move pointer */
unsigned int dns_read_int(unsigned char **buffer)
{
	unsigned int value;

	value = ntohl(*((unsigned int *)(*buffer)));
	*buffer += 4;

	return value;
}

/* iterator get rrs begin */
struct dns_rrs *dns_get_rrs_start(struct dns_packet *packet, dns_rr_type type, int *count)
{
	unsigned short start;
	struct dns_head *head = &packet->head;

	/* get rrs count by rrs type */
	switch (type) {
	case DNS_RRS_QD:
		*count = head->qdcount;
		start = packet->questions;
		break;
	case DNS_RRS_AN:
		*count = head->ancount;
		start = packet->answers;
		break;
	case DNS_RRS_NS:
		*count = head->nscount;
		start = packet->nameservers;
		break;
	case DNS_RRS_NR:
		*count = head->nrcount;
		start = packet->additional;
		break;
	default:
		return NULL;
		break;
	}

	/* if not resource record, reutrn null */
	if (start == DNS_RR_END) {
		return NULL;
	}

	/* return rrs data start address */
	return (struct dns_rrs *)(packet->data + start);
}

/* iterator next rrs */
struct dns_rrs *dns_get_rrs_next(struct dns_packet *packet, struct dns_rrs *rrs)
{
	if (rrs->next == DNS_RR_END) {
		return NULL;
	}

	return (struct dns_rrs *)(packet->data + rrs->next);
}

/* iterator add rrs begin */
unsigned char *_dns_add_rrs_start(struct dns_packet *packet, int *maxlen)
{
	struct dns_rrs *rrs;
	unsigned char *end = packet->data + packet->len;

	rrs = (struct dns_rrs *)end;
	*maxlen = packet->size - packet->len - sizeof(*packet);
	if (packet->len >= packet->size - sizeof(*packet)) {
		/* if size exceeds max packet size, return NULL */
		return NULL;
	}
	return rrs->data;
}

/* iterator add rrs end */
int dns_rr_add_end(struct dns_packet *packet, int type, dns_type_t rtype, int len)
{
	struct dns_rrs *rrs;
	struct dns_rrs *rrs_next;
	struct dns_head *head = &packet->head;
	unsigned char *end = packet->data + packet->len;
	unsigned short *count;
	unsigned short *start;

	rrs = (struct dns_rrs *)end;
	if (packet->len + len > packet->size - sizeof(*packet)) {
		return -1;
	}

	switch (type) {
	case DNS_RRS_QD:
		count = &head->qdcount;
		start = &packet->questions;
		break;
	case DNS_RRS_AN:
		count = &head->ancount;
		start = &packet->answers;
		break;
	case DNS_RRS_NS:
		count = &head->nscount;
		start = &packet->nameservers;
		break;
	case DNS_RRS_NR:
		count = &head->nrcount;
		start = &packet->additional;
		break;
	default:
		return -1;
		break;
	}

	/* add data to end of dns_packet, and set previouse rrs point to this rrs */
	if (*start != DNS_RR_END) {
		rrs_next = (struct dns_rrs *)(packet->data + *start);
		while (rrs_next->next != DNS_RR_END) {
			rrs_next = (struct dns_rrs *)(packet->data + rrs_next->next);
		}
		rrs_next->next = packet->len;
	} else {
		*start = packet->len;
	}

	/* update rrs head info */
	rrs->len = len;
	rrs->type = rtype;
	rrs->next = DNS_RR_END;

	/* update total data length */
	*count += 1;
	packet->len += len + sizeof(*rrs);
	return 0;
}

static inline int _dns_data_left_len(struct dns_data_context *data_context)
{
	/* check whether data length out of bound */
	return data_context->maxsize - (data_context->ptr - data_context->data);
}

int _dns_add_qr_head(struct dns_data_context *data_context, char *domain, int qtype, int qclass)
{
	/* question head */
	/* |domain         |
	 * |qtype | qclass |
	 */
	while (1) {
		if (_dns_data_left_len(data_context) < 1) {
			return -1;
		}
		*data_context->ptr = *domain;
		if (*domain == '\0') {
			data_context->ptr++;
			break;
		}
		data_context->ptr++;
		domain++;
	}

	if (_dns_data_left_len(data_context) < 4) {
		return -1;
	}

	*((unsigned short *)(data_context->ptr)) = qtype;
	data_context->ptr += 2;

	*((unsigned short *)(data_context->ptr)) = qclass;
	data_context->ptr += 2;

	return 0;
}

int _dns_get_qr_head(struct dns_data_context *data_context, char *domain, int maxsize, int *qtype, int *qclass)
{
	int i;
	/* question head */
	/* |domain         |
	 * |qtype | qclass |
	 */
	for (i = 0; i < maxsize; i++) {
		if (_dns_data_left_len(data_context) < 1) {
			return -1;
		}
		*domain = *data_context->ptr;
		if (*data_context->ptr == '\0') {
			domain++;
			data_context->ptr++;
			i++;
			break;
		}
		*domain = *data_context->ptr;
		domain++;
		data_context->ptr++;
	}

	if (_dns_data_left_len(data_context) < 4) {
		return -1;
	}

	*qtype = *((unsigned short *)(data_context->ptr));
	data_context->ptr += 2;

	*qclass = *((unsigned short *)(data_context->ptr));
	data_context->ptr += 2;

	return 0;
}

int _dns_add_rr_head(struct dns_data_context *data_context, char *domain, int qtype, int qclass, int ttl, int rr_len)
{
	int len = 0;

	/* resource record head */
	/* |domain          |
	 * |qtype  | qclass |
	 * |       ttl      |
	 * | rrlen | rrdata |
	 */
	len = _dns_add_qr_head(data_context, domain, qtype, qclass);
	if (len < 0) {
		return -1;
	}

	if (_dns_data_left_len(data_context) < 6) {
		return -1;
	}

	*((unsigned int *)(data_context->ptr)) = ttl;
	data_context->ptr += 4;

	*((unsigned short *)(data_context->ptr)) = rr_len;
	data_context->ptr += 2;

	return 0;
}

int _dns_get_rr_head(struct dns_data_context *data_context, char *domain, int maxsize, int *qtype, int *qclass, int *ttl, int *rr_len)
{
	int len = 0;

	/* resource record head */
	/* |domain          |
	 * |qtype  | qclass |
	 * |       ttl      |
	 * | rrlen | rrdata |
	 */
	len = _dns_get_qr_head(data_context, domain, maxsize, qtype, qclass);

	if (_dns_data_left_len(data_context) < 6) {
		return -1;
	}

	*ttl = *((unsigned int *)(data_context->ptr));
	data_context->ptr += 4;

	*rr_len = *((unsigned short *)(data_context->ptr));
	data_context->ptr += 2;

	return len;
}

int dns_add_RAW(struct dns_packet *packet, dns_rr_type rrtype, dns_type_t rtype, char *domain, int ttl, void *raw, int raw_len)
{
	int maxlen = 0;
	int len = 0;
	struct dns_data_context data_context;

	/* resource record */
	/* |domain          |
	 * |qtype  | qclass |
	 * |       ttl      |
	 * | rrlen | rrdata |
	 */
	unsigned char *data = _dns_add_rrs_start(packet, &maxlen);
	if (data == NULL) {
		return -1;
	}

	if (raw_len >= maxlen) {
		return -1;
	}

	data_context.data = data;
	data_context.ptr = data;
	data_context.maxsize = maxlen;

	/* add rr head */
	len = _dns_add_rr_head(&data_context, domain, rtype, DNS_C_IN, ttl, raw_len);
	if (len < 0) {
		return -1;
	}

	/* add rr data */
	memcpy(data_context.ptr, raw, raw_len);
	data_context.ptr += raw_len;
	len = data_context.ptr - data_context.data;

	return dns_rr_add_end(packet, rrtype, rtype, len);
}

int dns_get_RAW(struct dns_rrs *rrs, char *domain, int maxsize, int *ttl, void *raw, int *raw_len)
{
	int qtype = 0;
	int qclass = 0;
	int rr_len = 0;
	int ret = 0;
	struct dns_data_context data_context;

	/* resource record head */
	/* |domain          |
	 * |qtype  | qclass |
	 * |       ttl      |
	 * | rrlen | rrdata |
	 */
	unsigned char *data = rrs->data;

	data_context.data = data;
	data_context.ptr = data;
	data_context.maxsize = rrs->len;

	/* get rr head */
	ret = _dns_get_rr_head(&data_context, domain, maxsize, &qtype, &qclass, ttl, &rr_len);
	if (ret < 0) {
		return -1;
	}

	if (qtype != rrs->type || rr_len > *raw_len) {
		return -1;
	}

	/* get rr data */
	memcpy(raw, data_context.ptr, rr_len);
	data_context.ptr += rr_len;
	*raw_len = rr_len;

	return 0;
}

int dns_add_OPT(struct dns_packet *packet, dns_rr_type type, unsigned short opt_code, unsigned short opt_len, struct dns_opt *opt)
{
	// TODO
	
	int maxlen = 0;
	int len = 0;
	struct dns_data_context data_context;
	int total_len = sizeof(*opt) + opt->length;
	int ttl = 0;

	/*
    +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
  0: |                          OPTION-CODE                          |
     +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
  2: |                         OPTION-LENGTH                         |
     +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
  4: |                                                               |
     /                          OPTION-DATA                          /
     /                                                               /
     +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
	*/
	unsigned char *data = _dns_add_rrs_start(packet, &maxlen);
	if (data == NULL) {
		return -1;
	}

	if (total_len > maxlen) {
		return -1;
	}

	data_context.data = data;
	data_context.ptr = data;
	data_context.maxsize = maxlen;

	ttl = (opt_code << 16) | opt_len;

	/* add rr head */
	len = _dns_add_rr_head(&data_context, "", type, DNS_C_IN, ttl, total_len);
	if (len < 0) {
		return -1;
	}

	/* add rr data */
	memcpy(data_context.ptr, opt, total_len);
	data_context.ptr += total_len;
	len = data_context.ptr - data_context.data;

	return dns_rr_add_end(packet, type, DNS_T_OPT, len);
}

int dns_get_OPT(struct dns_rrs *rrs, unsigned short *opt_code, unsigned short *opt_len, struct dns_opt *opt, int *opt_maxlen)
{
	// TODO

	int qtype = 0;
	int qclass = 0;
	int rr_len = 0;
	int ret = 0;
	struct dns_data_context data_context;
	char domain[DNS_MAX_CNAME_LEN];
	int maxsize = DNS_MAX_CNAME_LEN;
	int ttl = 0;
	unsigned char *data = rrs->data;

	data_context.data = data;
	data_context.ptr = data;
	data_context.maxsize = rrs->len;

	/* get rr head */
	ret = _dns_get_rr_head(&data_context, domain, maxsize, &qtype, &qclass, &ttl, &rr_len);
	if (ret < 0) {
		return -1;
	}

	if (qtype != rrs->type || rr_len > *opt_len) {
		return -1;
	}

	/* get rr data */
	*opt_code = ttl >> 16;
	*opt_len = ttl & 0xFFFF;
	memcpy(opt, data_context.ptr, rr_len);
	data_context.ptr += rr_len;
	*opt_maxlen = rr_len;

	return 0;
}


int dns_add_CNAME(struct dns_packet *packet, dns_rr_type type, char *domain, int ttl, char *cname)
{
	int rr_len = strnlen(cname, DNS_MAX_CNAME_LEN) + 1;
	return dns_add_RAW(packet, type, DNS_T_CNAME, domain, ttl, cname, rr_len);
}

int dns_get_CNAME(struct dns_rrs *rrs, char *domain, int maxsize, int *ttl, char *cname, int cname_size)
{
	int len = cname_size;
	return dns_get_RAW(rrs, domain, maxsize, ttl, cname, &len);
}

int dns_add_A(struct dns_packet *packet, dns_rr_type type, char *domain, int ttl, unsigned char addr[DNS_RR_A_LEN])
{
	return dns_add_RAW(packet, type, DNS_T_A, domain, ttl, addr, DNS_RR_A_LEN);
}

int dns_get_A(struct dns_rrs *rrs, char *domain, int maxsize, int *ttl, unsigned char addr[DNS_RR_A_LEN])
{
	int len = DNS_RR_A_LEN;
	return dns_get_RAW(rrs, domain, maxsize, ttl, addr, &len);
}

int dns_add_PTR(struct dns_packet *packet, dns_rr_type type, char *domain, int ttl, char *cname)
{
	int rr_len = strnlen(cname, DNS_MAX_CNAME_LEN) + 1;
	return dns_add_RAW(packet, type, DNS_T_PTR, domain, ttl, cname, rr_len);
}

int dns_get_PTR(struct dns_rrs *rrs, char *domain, int maxsize, int *ttl, char *cname, int cname_size)
{
	int len = cname_size;
	return dns_get_RAW(rrs, domain, maxsize, ttl, cname, &len);
}

int dns_add_NS(struct dns_packet *packet, dns_rr_type type, char *domain, int ttl, char *cname)
{
	int rr_len = strnlen(cname, DNS_MAX_CNAME_LEN) + 1;
	return dns_add_RAW(packet, type, DNS_T_NS, domain, ttl, cname, rr_len);
}

int dns_get_NS(struct dns_rrs *rrs, char *domain, int maxsize, int *ttl, char *cname, int cname_size)
{
	int len = cname_size;
	return dns_get_RAW(rrs, domain, maxsize, ttl, cname, &len);
}

int dns_add_AAAA(struct dns_packet *packet, dns_rr_type type, char *domain, int ttl, unsigned char addr[DNS_RR_AAAA_LEN])
{
	return dns_add_RAW(packet, type, DNS_T_AAAA, domain, ttl, addr, DNS_RR_AAAA_LEN);
}

int dns_get_AAAA(struct dns_rrs *rrs, char *domain, int maxsize, int *ttl, unsigned char addr[DNS_RR_AAAA_LEN])
{
	int len = DNS_RR_AAAA_LEN;
	return dns_get_RAW(rrs, domain, maxsize, ttl, addr, &len);
}

int dns_add_SOA(struct dns_packet *packet, dns_rr_type type, char *domain, int ttl, struct dns_soa *soa)
{
	/* SOA */
	/*| mname        |
	 *| rname        |
	 *| serial       |
	 *| refersh      |
	 *| retry        |
	 *| expire       |
	 *| minimum      |
	 */
	unsigned char data[sizeof(*soa)];
	unsigned char *ptr = data;
	int len = 0;
	strncpy((char *)ptr, soa->mname, DNS_MAX_CNAME_LEN - 1);
	ptr += strnlen(soa->mname, DNS_MAX_CNAME_LEN - 1) + 1;
	strncpy((char *)ptr, soa->rname, DNS_MAX_CNAME_LEN - 1);
	ptr += strnlen(soa->rname, DNS_MAX_CNAME_LEN - 1) + 1;
	*((unsigned int *)ptr) = soa->serial;
	ptr += 4;
	*((unsigned int *)ptr) = soa->refresh;
	ptr += 4;
	*((unsigned int *)ptr) = soa->retry;
	ptr += 4;
	*((unsigned int *)ptr) = soa->expire;
	ptr += 4;
	*((unsigned int *)ptr) = soa->minimum;
	ptr += 4;
	len = ptr - data;

	return dns_add_RAW(packet, type, DNS_T_SOA, domain, ttl, data, len);
}

int dns_get_SOA(struct dns_rrs *rrs, char *domain, int maxsize, int *ttl, struct dns_soa *soa)
{
	unsigned char data[sizeof(*soa)];
	unsigned char *ptr = data;
	int len = sizeof(data);

	/* SOA */
	/*| mname        |
	 *| rname        |
	 *| serial       |
	 *| refersh      |
	 *| retry        |
	 *| expire       |
	 *| minimum      |
	 */
	if (dns_get_RAW(rrs, domain, maxsize, ttl, data, &len) != 0) {
		return -1;
	}

	strncpy(soa->mname, (char *)ptr, DNS_MAX_CNAME_LEN - 1);
	ptr += strnlen(soa->mname, DNS_MAX_CNAME_LEN - 1) + 1;
	if (ptr - data >= len) {
		return -1;
	}
	strncpy(soa->rname, (char *)ptr, DNS_MAX_CNAME_LEN - 1);
	ptr += strnlen(soa->rname, DNS_MAX_CNAME_LEN - 1) + 1;
	if (ptr - data + 20 > len) {
		return -1;
	}
	soa->serial = *((unsigned int *)ptr);
	ptr += 4;
	soa->refresh = *((unsigned int *)ptr);
	ptr += 4;
	soa->retry = *((unsigned int *)ptr);
	ptr += 4;
	soa->expire = *((unsigned int *)ptr);
	ptr += 4;
	soa->minimum = *((unsigned int *)ptr);
	ptr += 4;

	return 0;
}

int dns_add_OPT_ECS(struct dns_packet *packet, dns_rr_type type, struct dns_opt_ecs *ecs)
{
	// TODO
	
	unsigned char opt_data[DNS_MAX_OPT_LEN];
	struct dns_opt *opt = (struct dns_opt *)opt_data;
	int len = 0;

	opt->code = DNS_OPT_T_ECS;
	opt->length = sizeof(*ecs);
	memcpy(opt->data, ecs, sizeof(*ecs));

	/* ecs size 4 + bit of address*/
	len = sizeof(*opt) + 4;
	len += (ecs->source_prefix / 8);
	len += (ecs->source_prefix % 8 > 0) ? 1 : 0;

	return dns_add_OPT(packet, type, DNS_OPT_T_ECS, len, opt);
}

int dns_get_OPT_ECS(struct dns_rrs *rrs, unsigned short *opt_code, unsigned short *opt_len, struct dns_opt_ecs *ecs)
{
	// TODO
	
	unsigned char opt_data[DNS_MAX_OPT_LEN];
	struct dns_opt *opt = (struct dns_opt *)opt_data;
	int len = sizeof(opt_data);

	if (dns_get_OPT(rrs, opt_code, opt_len, opt, &len) != 0) {
		return -1;
	}

	if (opt->code != DNS_OPT_T_ECS) {
		return -1;
	}

	memcpy(ecs, opt->data, opt->length);

	return 0;
}

/*
 * Format:
 * |DNS_NAME\0(string)|qtype(short)|qclass(short)|
 */
int dns_add_domain(struct dns_packet *packet, char *domain, int qtype, int qclass)
{
	int len = 0;
	int maxlen = 0;
	unsigned char *data = _dns_add_rrs_start(packet, &maxlen);
	struct dns_data_context data_context;

	if (data == NULL) {
		return -1;
	}

	data_context.data = data;
	data_context.ptr = data;
	data_context.maxsize = maxlen;

	len = _dns_add_qr_head(&data_context, domain, qtype, qclass);
	if (len < 0) {
		return -1;
	}

	len = data_context.ptr - data_context.data;

	return dns_rr_add_end(packet, DNS_RRS_QD, DNS_T_CNAME, len);
}

int dns_get_domain(struct dns_rrs *rrs, char *domain, int maxsize, int *qtype, int *qclass)
{
	struct dns_data_context data_context;
	unsigned char *data = rrs->data;

	if (rrs->type != DNS_T_CNAME) {
		return -1;
	}

	data_context.data = data;
	data_context.ptr = data;
	data_context.maxsize = rrs->len;

	return _dns_get_qr_head(&data_context, domain, maxsize, qtype, qclass);
}

static inline int _dns_left_len(struct dns_context *context)
{
	return context->maxsize - (context->ptr - context->data);
}

static int _dns_decode_head(struct dns_context *context)
{
	unsigned int fields;
	int len = 12;
	struct dns_head *head = &context->packet->head;

	if (_dns_left_len(context) < len) {
		return -1;
	}

	/*
	0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
	+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	|                      ID                       |
	+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	|QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
	+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	|                    QDCOUNT                    |
	+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	|                    ANCOUNT                    |
	+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	|                    NSCOUNT                    |
	+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	|                    ARCOUNT                    |
	+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	*/

	head->id = dns_read_short(&context->ptr);
	fields = dns_read_short(&context->ptr);
	head->qr = (fields & QR_MASK) >> 15;
	head->opcode = (fields & OPCODE_MASK) >> 11;
	head->aa = (fields & AA_MASK) >> 10;
	head->tc = (fields & TC_MASK) >> 9;
	head->rd = (fields & RD_MASK) >> 8;
	head->ra = (fields & RA_MASK) >> 7;
	head->rcode = (fields & RCODE_MASK) >> 0;
	head->qdcount = dns_read_short(&context->ptr);
	head->ancount = dns_read_short(&context->ptr);
	head->nscount = dns_read_short(&context->ptr);
	head->nrcount = dns_read_short(&context->ptr);

	return 0;
}

static int _dns_encode_head(struct dns_context *context)
{
	int len = 12;
	struct dns_head *head = &context->packet->head;

	if (_dns_left_len(context) < len) {
		return -1;
	}

	dns_write_short(&context->ptr, head->id);

	int fields = 0;
	fields |= (head->qr << 15) & QR_MASK;
	fields |= (head->opcode << 11) & OPCODE_MASK;
	fields |= (head->aa << 10) & AA_MASK;
	fields |= (head->tc << 9) & TC_MASK;
	fields |= (head->rd << 8) & RD_MASK;
	fields |= (head->ra << 7) & RA_MASK;
	fields |= (head->rcode << 0) & RCODE_MASK;
	dns_write_short(&context->ptr, fields);

	dns_write_short(&context->ptr, head->qdcount);
	dns_write_short(&context->ptr, head->ancount);
	dns_write_short(&context->ptr, head->nscount);
	dns_write_short(&context->ptr, head->nrcount);
	return len;
}

static int _dns_decode_domain(struct dns_context *context, char *output, int size)
{
	int output_len = 0;
	int copy_len = 0;
	int len = *(context->ptr);
	unsigned char *ptr = context->ptr;
	int is_compressed = 0;

	/*[len]string[len]string...[0]0 */
	while (1) {
		if (ptr > context->data + context->maxsize || ptr < context->data) {
			return -1;
		}
		len = *ptr;
		if (len == 0) {
			*output = 0;
			ptr++;
			break;
		}

		/* compressed domain */
		if (len >= 0xC0) {
			/*
			0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
			+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
			| 1  1|                OFFSET                   |
			+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
			*/
			/* read offset */
			len = dns_read_short(&ptr) & 0x3FFF;
			if (is_compressed == 0) {
				context->ptr = ptr;
			}
			ptr = context->data + len;
			if (context->maxsize - (ptr - context->data) < 0) {
				tlog(TLOG_ERROR, "length is not enouth %u:%ld, %p, %p", context->maxsize, (long)(ptr - context->data), context->ptr, context->data);
				return -1;
			}
			is_compressed = 1;
			continue;
		}

		/* change [len] to '.' */
		if (output_len > 0) {
			*output = '.';
			output++;
		}

		if (context->maxsize - (ptr - context->data) < 0) {
			tlog(TLOG_ERROR, "length is not enouth %u:%ld, %p, %p", context->maxsize, (long)(ptr - context->data), context->ptr, context->data);
			return -1;
		}

		ptr++;
		if (output_len < size - 1) {
			/* copy sub string */
			copy_len = (len < size - output_len) ? len : size - 1 - output_len;
			if (context->maxsize - (ptr - context->data) < 0) {
				tlog(TLOG_ERROR, "length is not enouth %u:%ld, %p, %p", context->maxsize, (long)(ptr - context->data), context->ptr, context->data);
				return -1;
			}
			memcpy(output, ptr, copy_len);
		}

		ptr += len;
		output += len;
		output_len += len;
	}

	if (is_compressed == 0) {
		context->ptr = ptr;
	}

	return 0;
}

static int _dns_encode_domain(struct dns_context *context, char *domain)
{
	int num = 0;
	int total_len = 0;
	unsigned char *ptr_num = context->ptr++;

	/*[len]string[len]string...[0]0 */
	while (_dns_left_len(context) > 1 && *domain != 0) {
		if (*domain == '.') {
			*ptr_num = num;
			num = 0;
			ptr_num = context->ptr;
			domain++;
			context->ptr++;
			continue;
		}
		*context->ptr = *domain;
		num++;
		context->ptr++;
		domain++;
		total_len++;
	}

	*ptr_num = num;
	if (total_len > 0) {
		/* if domain is '\0', [domain] is '\0' */
		*(context->ptr) = 0;
		context->ptr++;
	}
	return 0;
}

static int _dns_decode_qr_head(struct dns_context *context, char *domain, int domain_size, int *qtype, int *qclass)
{
	int ret = 0;
	/*
	0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F 
	+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	|                                               |
	/                                               /
	/                      NAME                     /
	|                                               |
	+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	|                      TYPE                     |
	+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	|                     CLASS                     |
	+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	*/
	ret = _dns_decode_domain(context, domain, domain_size);
	if (ret < 0) {
		tlog(TLOG_ERROR, "decode domain failed.");
		return -1;
	}

	if (_dns_left_len(context) < 4) {
		tlog(TLOG_ERROR, "left length is not enough, %s.", domain);
		return -1;
	}

	*qtype = dns_read_short(&context->ptr);
	*qclass = dns_read_short(&context->ptr);

	return 0;
}

static int _dns_encode_qr_head(struct dns_context *context, char *domain, int qtype, int qclass)
{
	int ret = 0;
	ret = _dns_encode_domain(context, domain);
	if (ret < 0) {
		return -1;
	}

	if (_dns_left_len(context) < 4) {
		return -1;
	}

	dns_write_short(&context->ptr, qtype);
	dns_write_short(&context->ptr, qclass);

	return 0;
}

static int _dns_decode_rr_head(struct dns_context *context, char *domain, int domain_size, int *qtype, int *qclass, int *ttl, int *rr_len)
{
	int len = 0;

	len = _dns_decode_qr_head(context, domain, domain_size, qtype, qclass);
	if (len < 0) {
		tlog(TLOG_ERROR, "decode qr head failed.");
		return -1;
	}

	if (_dns_left_len(context) < 6) {
		tlog(TLOG_ERROR, "left length is not enough.");
		return -1;
	}

	*ttl = dns_read_int(&context->ptr);
	*rr_len = dns_read_short(&context->ptr);

	return 0;
}

static int _dns_encode_rr_head(struct dns_context *context, char *domain, int qtype, int qclass, int ttl, int rr_len)
{
	int ret = 0;
	ret = _dns_encode_qr_head(context, domain, qtype, qclass);
	if (ret < 0) {
		return -1;
	}

	if (_dns_left_len(context) < 6) {
		return -1;
	}

	dns_write_int(&context->ptr, ttl);
	dns_write_short(&context->ptr, rr_len);

	return 0;
}

static int _dns_encode_raw(struct dns_context *context, struct dns_rrs *rrs)
{
	int ret;
	int qtype = 0;
	int qclass = 0;
	int ttl = 0;
	char domain[DNS_MAX_CNAME_LEN];
	int rr_len;
	struct dns_data_context data_context;
	/*
	0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F 
	+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	|                                               |
	/                                               /
	/                      NAME                     /
	|                                               |
	+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	|                      TYPE                     |
	+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	|                     CLASS                     |
	+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	|                      TTL                      |
	|                                               |
	+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	|                   RDLENGTH                    |
	+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--|
	/                     RDATA                     /
	/                                               /
	+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	*/

	data_context.data = rrs->data;
	data_context.ptr = rrs->data;
	data_context.maxsize = rrs->len;

	ret = _dns_get_rr_head(&data_context, domain, DNS_MAX_CNAME_LEN, &qtype, &qclass, &ttl, &rr_len);
	if (ret < 0) {
		return -1;
	}

	ret = _dns_encode_rr_head(context, domain, qtype, qclass, ttl, rr_len);
	if (ret < 0) {
		return -1;
	}

	if (_dns_left_len(context) < rr_len) {
		return -1;
	}

	memcpy(context->ptr, data_context.ptr, rr_len);
	context->ptr += rr_len;
	data_context.ptr += rr_len;

	return 0;
}

static int _dns_decode_raw(struct dns_context *context, unsigned char *raw, int len)
{
	if (_dns_left_len(context) < len) {
		return -1;
	}

	memcpy(raw, context->ptr, len);
	context->ptr += len;
	return 0;
}

int _dns_decode_CNAME(struct dns_context *context, char *cname, int cname_size)
{
	int ret = 0;
	ret = _dns_decode_domain(context, cname, cname_size);
	if (ret < 0) {
		return -1;
	}

	return 0;
}

int _dns_encode_CNAME(struct dns_context *context, struct dns_rrs *rrs)
{
	int ret;
	int qtype = 0;
	int qclass = 0;
	int ttl = 0;
	char domain[DNS_MAX_CNAME_LEN];
	int rr_len;
	struct dns_data_context data_context;

	data_context.data = rrs->data;
	data_context.ptr = rrs->data;
	data_context.maxsize = rrs->len;

	ret = _dns_get_rr_head(&data_context, domain, DNS_MAX_CNAME_LEN, &qtype, &qclass, &ttl, &rr_len);
	if (ret < 0) {
		return -1;
	}

	/* when code domain, len must plus 1, because of length at the begining */
	rr_len++;
	if (rr_len > rrs->len) {
		return -1;
	}

	ret = _dns_encode_rr_head(context, domain, qtype, qclass, ttl, rr_len);
	if (ret < 0) {
		return -1;
	}

	ret = _dns_encode_domain(context, (char *)data_context.ptr);
	if (ret < 0) {
		return -1;
	}

	data_context.ptr += strnlen((char *)(data_context.ptr), DNS_MAX_CNAME_LEN) + 1;

	return 0;
}

int _dns_decode_SOA(struct dns_context *context, struct dns_soa *soa)
{
	int ret = 0;
	ret = _dns_decode_domain(context, soa->mname, DNS_MAX_CNAME_LEN - 1);
	if (ret < 0) {
		return -1;
	}

	ret = _dns_decode_domain(context, soa->rname, DNS_MAX_CNAME_LEN - 1);
	if (ret < 0) {
		return -1;
	}

	if (_dns_left_len(context) < 20) {
		return -1;
	}

	soa->serial = dns_read_int(&context->ptr);
	soa->refresh = dns_read_int(&context->ptr);
	soa->retry = dns_read_int(&context->ptr);
	soa->expire = dns_read_int(&context->ptr);
	soa->minimum = dns_read_int(&context->ptr);

	return 0;
}

int _dns_encode_SOA(struct dns_context *context, struct dns_rrs *rrs)
{
	int ret;
	int qtype = 0;
	int qclass = 0;
	int ttl = 0;
	char domain[DNS_MAX_CNAME_LEN];
	int rr_len;
	struct dns_data_context data_context;

	data_context.data = rrs->data;
	data_context.ptr = rrs->data;
	data_context.maxsize = rrs->len;

	ret = _dns_get_rr_head(&data_context, domain, DNS_MAX_CNAME_LEN, &qtype, &qclass, &ttl, &rr_len);
	if (ret < 0) {
		return -1;
	}

	/* when code two domain, len must plus 2, because of length at the begining */
	rr_len += 2;
	if (rr_len > rrs->len) {
		return -1;
	}

	ret = _dns_encode_rr_head(context, domain, qtype, qclass, ttl, rr_len);
	if (ret < 0) {
		return -1;
	}

	/* mname */
	ret = _dns_encode_domain(context, (char *)data_context.ptr);
	if (ret < 0) {
		return -1;
	}

	data_context.ptr += strnlen((char *)(data_context.ptr), DNS_MAX_CNAME_LEN) + 1;

	/* rname */
	ret = _dns_encode_domain(context, (char *)data_context.ptr);
	if (ret < 0) {
		return -1;
	}

	data_context.ptr += strnlen((char *)(data_context.ptr), DNS_MAX_CNAME_LEN) + 1;

	if (_dns_left_len(context) < 20) {
		return -1;
	}

	dns_write_int(&context->ptr, *(unsigned int *)data_context.ptr);
	data_context.ptr += 4;
	dns_write_int(&context->ptr, *(unsigned int *)data_context.ptr);
	data_context.ptr += 4;
	dns_write_int(&context->ptr, *(unsigned int *)data_context.ptr);
	data_context.ptr += 4;
	dns_write_int(&context->ptr, *(unsigned int *)data_context.ptr);
	data_context.ptr += 4;
	dns_write_int(&context->ptr, *(unsigned int *)data_context.ptr);
	data_context.ptr += 4;

	return 0;
}


static int _dns_decode_opt_ecs(struct dns_context *context, struct dns_opt_ecs *ecs)
{
	// TODO
	
	int len = 0;
	if (_dns_left_len(context) < 4) {
		return -1;
	}

	ecs->family = dns_read_short(&context->ptr);
	ecs->source_prefix = dns_read_char(&context->ptr);
	ecs->scope_prefix = dns_read_char(&context->ptr);
	len = (ecs->source_prefix / 8);
	len += (ecs->source_prefix % 8 > 0) ? 1 : 0;

	if (_dns_left_len(context) < len) {
		return -1;
	}

	memcpy(ecs->addr, context->ptr, len);
	context->ptr += len;

	return 0;
}

int _dns_encode_OPT(struct dns_context *context, struct dns_rrs *rrs)
{
	// TODO
	
	return 0;
}

static int _dns_decode_opt(struct dns_context *context, dns_rr_type type, unsigned int ttl, int rr_len)
{
	unsigned short opt_code;
	unsigned short opt_len;
	unsigned short ercode = (ttl >> 16) & 0xFFFF;
	unsigned short ever = (ttl) & 0xFFFF;
	unsigned char *start = context->ptr;
	struct dns_packet *packet = context->packet;
	int ret = 0;
	/*
	     Field Name   Field Type     Description
     ------------------------------------------------------
     NAME         domain name    empty (root domain)
     TYPE         u_int16_t      OPT
     CLASS        u_int16_t      sender's UDP payload size
     TTL          u_int32_t      extended RCODE and flags
     RDLEN        u_int16_t      describes RDATA
     RDATA        octet stream   {attribute,value} pairs

	                 +0 (MSB)                            +1 (LSB)
     +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
  0: |                          OPTION-CODE                          |
     +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
  2: |                         OPTION-LENGTH                         |
     +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
  4: |                                                               |
     /                          OPTION-DATA                          /
     /                                                               /
     +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

	TTL
                 +0 (MSB)                            +1 (LSB)
      +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
   0: |         EXTENDED-RCODE        |            VERSION            |
      +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
   2: |                               Z                               |
      +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
	*/

	if (ercode != 0) {
		tlog(TLOG_ERROR, "extend rcode invalid.");
		return -1;
	}
	ever = ever;

	while (context->ptr - start < rr_len) {
		opt_code = dns_read_short(&context->ptr);
		opt_len = dns_read_short(&context->ptr);
		switch (opt_code) {
		case DNS_OPT_T_ECS: {
			struct dns_opt_ecs ecs;
			ret = _dns_decode_opt_ecs(context, &ecs);
			if (ret != 0 ) {
				tlog(TLOG_ERROR, "decode ecs failed.");
				return -1;
			}

			ret = dns_add_OPT_ECS(packet, type, &ecs);
		} break;
		default:
			context->ptr += opt_len;
			tlog(TLOG_DEBUG, "DNS opt type = %d not supported", opt_code);
			break;
		}				
	}

	return 0;
}

static int _dns_decode_qd(struct dns_context *context)
{
	struct dns_packet *packet = context->packet;
	int len;
	int qtype = 0;
	int qclass = 0;
	char domain[DNS_MAX_CNAME_LEN];

	len = _dns_decode_qr_head(context, domain, DNS_MAX_CNAME_LEN, &qtype, &qclass);
	if (len < 0) {
		return -1;
	}

	len = dns_add_domain(packet, domain, qtype, qclass);
	if (len < 0) {
		return -1;
	}

	return 0;
}

static int _dns_decode_an(struct dns_context *context, dns_rr_type type)
{
	int ret;
	int qtype = 0;
	int qclass = 0;
	int ttl;
	int rr_len = 0;
	char domain[DNS_MAX_CNAME_LEN];
	struct dns_packet *packet = context->packet;
	unsigned char *start;

	/* decode rr head */
	ret = _dns_decode_rr_head(context, domain, DNS_MAX_CNAME_LEN, &qtype, &qclass, &ttl, &rr_len);
	if (ret < 0) {
		tlog(TLOG_ERROR, "decode head failed.");
		return -1;
	}
	start = context->ptr;

	/* decode answer */
	switch (qtype) {
	case DNS_T_A: {
		unsigned char addr[DNS_RR_A_LEN];
		ret = _dns_decode_raw(context, addr, sizeof(addr));
		if (ret < 0) {
			tlog(TLOG_ERROR, "decode A failed, %s", domain);
			return -1;
		}

		ret = dns_add_A(packet, type, domain, ttl, addr);
		if (ret < 0) {
			tlog(TLOG_ERROR, "add A failed, %s", domain);
			return -1;
		}
	} break;
	case DNS_T_CNAME: {
		char cname[DNS_MAX_CNAME_LEN];
		ret = _dns_decode_CNAME(context, cname, DNS_MAX_CNAME_LEN);
		if (ret < 0) {
			tlog(TLOG_ERROR, "decode CNAME failed, %s", domain);
			return -1;
		}

		ret = dns_add_CNAME(packet, type, domain, ttl, cname);
		if (ret < 0) {
			tlog(TLOG_ERROR, "add CNAME failed, %s", domain);
			return -1;
		}
	} break;
	case DNS_T_SOA: {
		struct dns_soa soa;
		ret = _dns_decode_SOA(context, &soa);
		if (ret < 0) {
			tlog(TLOG_ERROR, "decode CNAME failed, %s", domain);
			return -1;
		}

		ret = dns_add_SOA(packet, type, domain, ttl, &soa);
		if (ret < 0) {
			tlog(TLOG_ERROR, "add CNAME failed, %s", domain);
			return -1;
		}
	} break;
	case DNS_T_NS: {
		char ns[DNS_MAX_CNAME_LEN];
		ret = _dns_decode_CNAME(context, ns, DNS_MAX_CNAME_LEN);
		if (ret < 0) {
			tlog(TLOG_ERROR, "decode NS failed, %s", domain);
			return -1;
		}

		ret = dns_add_NS(packet, type, domain, ttl, ns);
		if (ret < 0) {
			tlog(TLOG_ERROR, "add NS failed, %s", domain);
			return -1;
		}
	} break;
	case DNS_T_PTR: {
		char name[DNS_MAX_CNAME_LEN];
		ret = _dns_decode_CNAME(context, name, DNS_MAX_CNAME_LEN);
		if (ret < 0) {
			tlog(TLOG_ERROR, "decode PTR failed, %s", domain);
			return -1;
		}

		ret = dns_add_PTR(packet, type, domain, ttl, name);
		if (ret < 0) {
			tlog(TLOG_ERROR, "add PTR failed, %s", domain);
			return -1;
		}
	} break;
	case DNS_T_AAAA: {
		unsigned char addr[DNS_RR_AAAA_LEN];
		ret = _dns_decode_raw(context, addr, sizeof(addr));
		if (ret < 0) {
			tlog(TLOG_ERROR, "decode AAAA failed, %s", domain);
			return -1;
		}

		ret = dns_add_AAAA(packet, type, domain, ttl, addr);
		if (ret < 0) {
			tlog(TLOG_ERROR, "add AAAA failed, %s", domain);
			return -1;
		}
	} break;
	case DNS_T_OPT: {
		unsigned char  *opt_start = context->ptr;
		ret = _dns_decode_opt(context, type, ttl, rr_len);
		if (ret < 0) {
			tlog(TLOG_ERROR, "decode opt failed, %s", domain);
			return -1;
		}

		if (context->ptr - opt_start != rr_len) {
			tlog(TLOG_ERROR, "opt length mitchmatch, %s\n", domain);
			return -1;
		}
	} break;
	default:
		context->ptr += rr_len;
		tlog(TLOG_DEBUG, "DNS type = %d not supported", qtype);
		break;
	}

	if (context->ptr - start != rr_len) {
		tlog(TLOG_ERROR, "length mitchmatch , %s, %ld:%d", domain, (long)(context->ptr - start), rr_len);
		return -1;
	}

	return 0;
}

static int _dns_encode_qd(struct dns_context *context, struct dns_rrs *rrs)
{
	int ret;
	int qtype = 0;
	int qclass = 0;
	char domain[DNS_MAX_CNAME_LEN];
	struct dns_data_context data_context;

	data_context.data = rrs->data;
	data_context.ptr = rrs->data;
	data_context.maxsize = rrs->len;

	ret = _dns_get_qr_head(&data_context, domain, DNS_MAX_CNAME_LEN, &qtype, &qclass);
	if (ret < 0) {
		return -1;
	}

	ret = _dns_encode_qr_head(context, domain, qtype, qclass);
	if (ret < 0) {
		return -1;
	}

	return 0;
}

static int _dns_encode_an(struct dns_context *context, struct dns_rrs *rrs)
{
	int ret;
	switch (rrs->type) {
	case DNS_T_A:
	case DNS_T_AAAA: {
		ret = _dns_encode_raw(context, rrs);
		if (ret < 0) {
			return -1;
		}
	} break;
	case DNS_T_CNAME:
	case DNS_T_PTR:
		ret = _dns_encode_CNAME(context, rrs);
		if (ret < 0) {
			return -1;
		}
		break;
	case DNS_T_SOA:
		ret = _dns_encode_SOA(context, rrs);
		if (ret < 0) {
			return -1;
		}
		break;
	case DNS_T_OPT:
		ret = _dns_encode_OPT(context, rrs);
		if (ret < 0) {
			return -1;
		}
		break;
	default:
		break;
	}

	return 0;
}

static int _dns_decode_body(struct dns_context *context)
{
	struct dns_packet *packet = context->packet;
	struct dns_head *head = &packet->head;
	int i = 0;
	int ret = 0;

	for (i = 0; i < head->qdcount; i++) {
		ret = _dns_decode_qd(context);
		if (ret < 0) {
			tlog(TLOG_ERROR, "decode qd failed.");
			return -1;
		}
		head->qdcount--;
	}

	for (i = 0; i < head->ancount; i++) {
		ret = _dns_decode_an(context, DNS_RRS_AN);
		if (ret < 0) {
			tlog(TLOG_ERROR, "decode an failed.");
			return -1;
		}
		head->ancount--;
	}

	for (i = 0; i < head->nscount; i++) {
		ret = _dns_decode_an(context, DNS_RRS_NS);
		if (ret < 0) {
			tlog(TLOG_ERROR, "decode ns failed.");
			return -1;
		}
		head->nscount--;
	}

	for (i = 0; i < head->nrcount; i++) {
		ret = _dns_decode_an(context, DNS_RRS_NR);
		if (ret < 0) {
			tlog(TLOG_ERROR, "decode nr failed.");
			return -1;
		}
		head->nrcount--;
	}

	return 0;
}

static int _dns_encode_body(struct dns_context *context)
{
	struct dns_packet *packet = context->packet;
	struct dns_head *head = &packet->head;
	int i = 0;
	int len = 0;
	struct dns_rrs *rrs;
	int count;

	rrs = dns_get_rrs_start(packet, DNS_RRS_QD, &count);
	head->qdcount = count;
	for (i = 0; i < count && rrs; i++, rrs = dns_get_rrs_next(packet, rrs)) {
		len = _dns_encode_qd(context, rrs);
		if (len < 0) {
			return -1;
		}
	}

	rrs = dns_get_rrs_start(packet, DNS_RRS_AN, &count);
	head->ancount = count;
	for (i = 0; i < count && rrs; i++, rrs = dns_get_rrs_next(packet, rrs)) {
		len = _dns_encode_an(context, rrs);
		if (len < 0) {
			return -1;
		}
	}

	rrs = dns_get_rrs_start(packet, DNS_RRS_NS, &count);
	head->nscount = count;
	for (i = 0; i < count && rrs; i++, rrs = dns_get_rrs_next(packet, rrs)) {
		len = _dns_encode_an(context, rrs);
		if (len < 0) {
			return -1;
		}
	}

	rrs = dns_get_rrs_start(packet, DNS_RRS_NR, &count);
	head->nrcount = count;
	for (i = 0; i < count && rrs; i++, rrs = dns_get_rrs_next(packet, rrs)) {
		len = _dns_encode_an(context, rrs);
		if (len < 0) {
			return -1;
		}
	}

	return 0;
}

int dns_packet_init(struct dns_packet *packet, int size, struct dns_head *head)
{
	struct dns_head *init_head = &packet->head;
	memset(packet, 0, size);
	packet->size = size;
	init_head->id = head->id;
	init_head->qr = head->qr;
	init_head->opcode = head->opcode;
	init_head->aa = head->aa;
	init_head->tc = head->tc;
	init_head->rd = head->rd;
	init_head->ra = head->ra;
	init_head->rcode = head->rcode;
	packet->questions = DNS_RR_END;
	packet->answers = DNS_RR_END;
	packet->nameservers = DNS_RR_END;
	packet->additional = DNS_RR_END;

	return 0;
}

int dns_decode(struct dns_packet *packet, int maxsize, unsigned char *data, int size)
{
	struct dns_head *head = &packet->head;
	struct dns_context context;
	int ret = 0;

	memset(&context, 0, sizeof(context));
	memset(packet, 0, sizeof(*packet));

	context.data = data;
	context.packet = packet;
	context.ptr = data;
	context.maxsize = size;

	dns_packet_init(packet, maxsize, head);
	ret = _dns_decode_head(&context);
	if (ret < 0) {
		return -1;
	}

	ret = _dns_decode_body(&context);
	if (ret < 0) {
		tlog(TLOG_ERROR, "decode body failed.\n");
		return -1;
	}

	return 0;
}

int dns_encode(unsigned char *data, int size, struct dns_packet *packet)
{
	int ret = 0;
	struct dns_context context;

	memset(&context, 0, sizeof(context));
	context.data = data;
	context.packet = packet;
	context.ptr = data;
	context.maxsize = size;

	ret = _dns_encode_head(&context);
	if (ret < 0) {
		return -1;
	}

	ret = _dns_encode_body(&context);
	if (ret < 0) {
		return -1;
	}

	return context.ptr - context.data;
}

void dns_debug(void)
{
	unsigned char data[1024];
	int len;
	char buff[4096];

	int fd = open("dns.bin", O_RDWR);
	if (fd < 0) {
		return;
	}
	len = read(fd, data, 1024);
	close(fd);
	if (len < 0) {
		return;
	}

	struct dns_packet *packet = (struct dns_packet *)buff;
	if (dns_decode(packet, 4096, data, len) != 0) {
		tlog(TLOG_ERROR, "decode failed.\n");
	}

	memset(data, 0, sizeof(data));
	len = dns_encode(data, 1024, packet);
	if (len < 0) {
		tlog(TLOG_ERROR, "encode failed.\n");
	}

	fd = open("dns-cmp.bin", O_CREAT | O_TRUNC | O_RDWR);
	write(fd, data, len);
	close(fd);
}