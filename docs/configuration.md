---
hide:
  - navigation
  - toc
---

# 配置选项

配置建议：

**smartdns默认已设置为最优模式，适合大部分场景的DNS查询体验改善，一般情况只需要增加上游服务器地址即可，无需做其他配置修改；如有其他配置修改，请务必了解其用途，避免修改后起到反作用。**

| 键名 | 功能说明 | 默认值 | 可用值/要求 | 举例 |
| :--- | :--- | :--- | :--- | :--- |
| server-name | DNS 服务器名称 | 操作系统主机名 / smartdns | 符合主机名规格的字符串 | server-name smartdns |
| bind | DNS 监听端口号  | [::]:53 | 可绑定多个端口。<br />IP:PORT@DEVICE: 服务器 IP:端口号@设备名<br />[-group]: 请求时使用的 DNS 服务器组<br />[-no-rule-addr]：跳过 address 规则<br />[-no-rule-nameserver]：跳过 Nameserver 规则<br />[-no-rule-ipset]：跳过 ipset 和 nftset 规则<br />[-no-rule-soa]：跳过 SOA(#) 规则<br />[-no-dualstack-selection]：停用双栈测速<br />[-no-speed-check]：停用测速<br />[-no-cache]：停止缓存 <br />[-force-aaaa-soa]: 禁用IPV6查询 | bind :53@eth0 |
| bind-tcp | DNS TCP 监听端口号 | [::]:53 | 可绑定多个端口。<br />IP:PORT@DEVICE: 服务器 IP:端口号@设备名<br />[-group]: 请求时使用的 DNS 服务器组<br />[-no-rule-addr]：跳过 address 规则<br />[-no-rule-nameserver]：跳过 nameserver 规则<br />[-no-rule-ipset]：跳过 ipset 和 nftset 规则。<br />[-no-rule-soa]：跳过 SOA(#) 规则<br />[-no-dualstack-selection]：停用双栈测速<br />[-no-speed-check]：停用测速<br />[-no-cache]：停止缓存 <br />[-force-aaaa-soa]: 禁用IPV6查询  | bind-tcp :53 |
| bind-tls | DNS Over TLS 监听端口号 | [::]:853 | 可绑定多个端口。<br />IP:PORT@DEVICE: 服务器 IP:端口号@设备名<br />[-group]: 请求时使用的 DNS 服务器组<br />[-no-rule-addr]：跳过 address 规则<br />[-no-rule-nameserver]：跳过 nameserver 规则<br />[-no-rule-ipset]：跳过 ipset 和 nftset 规则。<br />[-no-rule-soa]：跳过 SOA(#) 规则<br />[-no-dualstack-selection]：停用双栈测速<br />[-no-speed-check]：停用测速<br />[-no-cache]：停止缓存 <br />[-force-aaaa-soa]: 禁用IPV6查询 | bind-tls :853 |
| bind-cert-file | SSL证书文件路径 | smartdns-cert.pem | 合法路径字符串 | bind-cert-file cert.pem |
| bind-cert-key-file | SSL证书KEY文件路径 | smartdns-key.pem | 合法路径字符串 | bind-cert-key-file key.pem |
| bind-cert-key-pass | SSL证书KEY文件密码 | 无 | 字符串 | bind-cert-key-pass password |
| cache-size | 域名结果缓存个数 | 自动，根据系统内存自动调整大小 | 大于等于 0 的数字 | cache-size 512 |
| cache-persist | 是否持久化缓存 | 自动。<br />当 cache-file 所在的位置有超过 128 MB 的可用空间时启用，否则禁用。 | [yes\|no] | cache-persist yes |
| cache-file | 缓存持久化文件路径 | /tmp/<br />smartdns.cache | 合法路径字符串 | cache-file /tmp/smartdns.cache |
| tcp-idle-time | TCP 链接空闲超时时间 | 120 | 大于等于 0 的数字 | tcp-idle-time 120 |
| rr-ttl | 域名结果 TTL | 远程查询结果 | 大于 0 的数字 | rr-ttl 600 |
| rr-ttl-min | 允许的最小 TTL 值 | 远程查询结果 | 大于 0 的数字 | rr-ttl-min 60 |
| rr-ttl-max | 允许的最大 TTL 值 | 远程查询结果 | 大于 0 的数字 | rr-ttl-max 600 |
| rr-ttl-reply-max | 允许返回给客户端的最大 TTL 值 | 远程查询结果 | 大于 0 的数字 | rr-ttl-reply-max 60 |
| local-ttl | 本地HOST，address的TTL值 | rr-ttl-min | 大于 0 的数字 | local-ttl  60 |
| max-reply-ip-num | 允许返回给客户的最大IP数量 | IP数量 | 大于 0 的数字 | max-reply-ip-num 1 |
| log-level | 设置日志级别 | error | fatal、error、warn、notice、info 或 debug | log-level error |
| log-file | 日志文件路径 | /var/log/<br />smartdns/<br />smartdns.log | 合法路径字符串 | log-file /var/log/smartdns/smartdns.log |
| log-size | 日志大小 | 128K | 数字 + K、M 或 G | log-size 128K |
| log-num | 日志归档个数 | openwrt为2， 其他系统为8 | 大于等于 0 的数字，0表示禁用日志 | log-num 2 |
| log-file-mode | 日志归档文件权限 | 0640 | 文件权限 | log-file-mode 644 |
| log-console | 是否输出日志到控制台 | no | [yes\|no] | log-console yes |
| audit-enable | 设置审计启用 | no | [yes\|no] | audit-enable yes |
| audit-file | 审计文件路径 | /var/log/<br />smartdns/<br />smartdns-audit.log | 合法路径字符串 | audit-file /var/log/smartdns/smartdns-audit.log |
| audit-size | 审计大小 | 128K | 数字 + K、M 或 G | audit-size 128K |
| audit-num | 审计归档个数 | 2 | 大于等于 0 的数字 | audit-num 2 |
| audit-file-mode | 审计归档文件权限 | 0640 | 文件权限 | log-file-mode 644 |
| audit-console | 是否输出审计日志到控制台 | no | [yes\|no] | audit-console yes |
| conf-file | 附加配置文件 | 无 | 合法路径字符串 | conf-file /etc/smartdns/smartdns.more.conf |
| server | 上游 UDP DNS | 无 | 可重复。<br />[ip][:port]\|URL：服务器 IP:端口（可选）或 URL <br />[-blacklist-ip]：配置 IP 过滤结果。<br />[-whitelist-ip]：指定仅接受参数中配置的 IP 范围<br />[-group [group] ...]：DNS 服务器所属组，比如 office 和 foreign，和 nameserver 配套使用<br />[-exclude-default-group]：将 DNS 服务器从默认组中排除。<br />[-set-mark mark]：设置数据包标记so-mark。<br />[-proxy name]：设置代理服务器。 <br />[-bootstrap-dns]：标记此服务器为bootstrap服务器。<br />[-subnet]：指定服务器使用的edns-client-subnet| server 8.8.8.8:53 -blacklist-ip -group g1 -proxy proxy<br /> server tls://8.8.8.8|
| server-tcp | 上游 TCP DNS | 无 | 可重复。<br />[ip][:port]：服务器 IP:端口（可选）<br />[-blacklist-ip]：配置 IP 过滤结果<br />[-whitelist-ip]：指定仅接受参数中配置的 IP 范围。<br />[-group [group] ...]：DNS 服务器所属组，比如 office 和 foreign，和 nameserver 配套使用<br />[-exclude-default-group]：将 DNS 服务器从默认组中排除。<br />[-set-mark mark]：设置数据包标记so-mark。<br />[-proxy name]：设置代理服务器。 <br />[-bootstrap-dns]：标记此服务器为bootstrap服务器。<br />[-subnet]：指定服务器使用的edns-client-subnet| server-tcp 8.8.8.8:53 |
| server-tls | 上游 TLS DNS | 无 | 可重复。<br />[ip][:port]：服务器 IP:端口（可选)<br />[-spki-pin [sha256-pin]]：TLS 合法性校验 SPKI 值，base64 编码的 sha256 SPKI pin 值<br />[-host-name]：TLS SNI 名称, 名称设置为-，表示停用SNI名称<br />[-tls-host-verify]：TLS 证书主机名校验<br /> [-no-check-certificate]：跳过证书校验<br />[-blacklist-ip]：配置 IP 过滤结果<br />[-whitelist-ip]：仅接受参数中配置的 IP 范围<br />[-group [group] ...]：DNS 服务器所属组，比如 office 和 foreign，和 nameserver 配套使用<br />[-exclude-default-group]：将 DNS 服务器从默认组中排除。<br />[-set-mark mark]：设置数据包标记so-mark。<br />[-proxy name]：设置代理服务器。 <br />[-bootstrap-dns]：标记此服务器为bootstrap服务器。<br />[-subnet]：指定服务器使用的edns-client-subnet| server-tls 8.8.8.8:853 |
| server-https | 上游 HTTPS DNS | 无 | 可重复。<br />https://[host>][:port]/path：服务器 IP:端口（可选）<br />[-spki-pin [sha256-pin]]：TLS 合法性校验 SPKI 值，base64 编码的 sha256 SPKI pin 值<br />[-host-name]：TLS SNI 名称<br />[-http-host]：http 协议头主机名<br />[-tls-host-verify]：TLS 证书主机名校验<br /> [-no-check-certificate]：跳过证书校验<br />[-blacklist-ip]：配置 IP 过滤结果<br />[-whitelist-ip]：仅接受参数中配置的 IP 范围。<br />[-group [group] ...]：DNS 服务器所属组，比如 office 和 foreign，和 nameserver 配套使用<br />[-exclude-default-group]：将 DNS 服务器从默认组中排除。<br />[-set-mark]：设置数据包标记so-mark。<br />[-proxy name]：设置代理服务器。 <br />[-bootstrap-dns]：标记此服务器为bootstrap服务器。<br />[-subnet]：指定服务器使用的edns-client-subnet| server-https https://cloudflare-dns.com/dns-query |
| proxy-server | 代理服务器 | 无 | 可重复。<br />proxy-server URL <br />[URL]: [socks5\|http]://[username:password@]host:port<br />[-name]: 代理服务器名称。 |proxy-server socks5://user:pass@1.2.3.4:1080 -name proxy|
| speed-check-mode | 测速模式选择 | 无 | [ping\|tcp:[80]\|none] | speed-check-mode ping,tcp:80,tcp:443 |
| response-mode | 首次查询响应模式 | first-ping |模式：[first-ping\|fastest-ip\|fastest-response]<br /> [first-ping]: 最快ping响应地址模式，DNS上游最快查询时延+ping时延最短，查询等待与链接体验最佳;<br />[fastest-ip]: 最快IP地址模式，查询到的所有IP地址中ping最短的IP。需等待IP测速; <br />[fastest-response]: 最快响应的DNS结果，DNS查询等待时间最短，返回的IP地址可能不是最快。| response-mode first-ping |
| expand-ptr-from-address | 是否扩展Address对应的PTR记录 | no | [yes\|no] | expand-ptr-from-address yes |
| address | 指定域名 IP 地址 | 无 | address /domain/[ip\|-\|-4\|-6\|#\|#4\|#6] <br />- 表示忽略 <br /># 表示返回 SOA <br />4 表示 IPv4 <br />6 表示 IPv6 | address /www.example.com/1.2.3.4 |
| cname | 指定域名别名 | 无 | cname /domain/target <br />- 表示忽略 <br />指定对应域名的cname | cname /www.example.com/cdn.example.com |
| dns64 | DNS64转换 | 无 | dns64 ip-prefix/mask <br /> ipv6前缀和掩码 | dns64 64:ff9b::/96 |
| edns-client-subnet | DNS ECS | 无 | edns-client-subnet ip-prefix/mask <br /> 指定EDNS客户端子网 | ip-prefix/mask 1.2.3.4/23 |
| nameserver | 指定域名使用 server 组解析 | 无 | nameserver /domain/[group\|-], group 为组名，- 表示忽略此规则，配套 server 中的 -group 参数使用 | nameserver /www.example.com/office |
| ipset | 域名 ipset | 无 | ipset /domain/[ipset\|-\|#[4\|6]:[ipset\|-][,#[4\|6]:[ipset\|-]]]，-表示忽略 | ipset /www.example.com/#4:dns4,#6:- <br />ipset /www.example.com/dns |
| ipset-timeout | 设置 ipset 超时功能启用  | no | [yes\|no] | ipset-timeout yes |
| ipset-no-speed | 当测速失败时，将域名结果设置到ipset集合中 | 无 | ipset \| #[4\|6]:ipset | ipset-no-speed #4:ipset4,#6:ipset6 <br /> ipset-no-speed ipset|
| nftset | 域名 nftset | 无 | nftset /domain/[#4\|#6\|-]:[family#nftable#nftset\|-][,#[4\|6]:[family#nftable#nftset\|-]]]，<br />-表示忽略；<br />ipv4 地址的 family 只支持 inet 和 ip；<br />ipv6 地址的 family 只支持 inet 和 ip6；<br />由于 nft 限制，两种地址只能分开存放于两个 set 中。| nftset /www.example.com/#4:inet#tab#dns4,#6:- |
| nftset-timeout | 设置 nftset 超时功能启用  | no | [yes\|no] | nftset-timeout yes |
| nftset-no-speed | 当测速失败时，将域名结果设置到nftset集合中 | 无 | nftset-no-speed [#4\|#6]:[family#nftable#nftset][,#[4\|6]:[family#nftable#nftset]]] <br />ipv4 地址的 family 只支持 inet 和 ip <br />ipv6 地址的 family 只支持 inet 和 ip6 <br />由于 nft 限制，两种地址只能分开存放于两个 set 中。| nftset-no-speed #4:inet#tab#set4|
| nftset-debug | 设置 nftset 调试功能启用  | no | [yes\|no] | nftset-debug yes |
| domain-rules | 设置域名规则 | 无 | domain-rules /domain/ [-rules...]<br />[-c\|-speed-check-mode]：测速模式，参考 speed-check-mode 配置<br />[-a\|-address]：参考 address 配置<br />[-n\|-nameserver]：参考 nameserver 配置<br />[-p\|-ipset]：参考ipset配置<br />[-t\|-nftset]：参考nftset配置<br />[-d\|-dualstack-ip-selection]：参考 dualstack-ip-selection<br /> [-no-serve-expired]：禁用过期缓存<br />[-rr-ttl\|-rr-ttl-min\|-rr-ttl-max]: 参考配置rr-ttl, rr-ttl-min, rr-ttl-max<br />[-no-cache]：不缓存当前域名<br />[-r\|-response-mode]：响应模式，参考 response-mode 配置<br />[-delete]：删除对应的规则 | domain-rules /www.example.com/ -speed-check-mode none |
| domain-set | 设置域名集合 | 无 | domain-set [options...]<br />[-n\|-name]：域名集合名称 <br />[-t\|-type]：域名集合类型，当前仅支持list，格式为域名列表，一行一个域名。<br />[-f\|-file]：域名集合文件路径。<br /> 选项需要配合address, nameserver, ipset, nftset等需要指定域名的地方使用，使用方式为 /domain-set:[name]/| domain-set -name set -type list -file /path/to/list <br /> address /domain-set:set/1.2.4.8 |
| bogus-nxdomain | 假冒 IP 地址过滤 | 无 | [ip/subnet]，可重复 | bogus-nxdomain 1.2.3.4/16 |
| ignore-ip | 忽略 IP 地址 | 无 | [ip/subnet]，可重复 | ignore-ip 1.2.3.4/16 |
| whitelist-ip | 白名单 IP 地址 | 无 | [ip/subnet]，可重复 | whitelist-ip 1.2.3.4/16 |
| blacklist-ip | 黑名单 IP 地址 | 无 | [ip/subnet]，可重复 | blacklist-ip 1.2.3.4/16 |
| force-AAAA-SOA | 强制 AAAA 地址返回 SOA | no | [yes\|no] | force-AAAA-SOA yes |
| force-qtype-SOA | 强制指定 qtype 返回 SOA | qtype id | [qtypeid\|...] | force-qtype-SOA 65 28
| prefetch-domain | 域名预先获取功能 | no | [yes\|no] | prefetch-domain yes |
| dnsmasq-lease-file | 支持读取dnsmasq dhcp文件解析本地主机名功能 | 无 | dnsmasq dhcp lease文件路径 | dnsmasq-lease-file /var/lib/misc/dnsmasq.leases |
| serve-expired | 过期缓存服务功能 | yes | [yes\|no]，开启此功能后，如果有请求时尝试回应 TTL 为 0 的过期记录，并发查询记录，以避免查询等待 |
| serve-expired-ttl | 过期缓存服务最长超时时间 | 0 | 秒，0 表示停用超时，大于 0 表示指定的超时的秒数 | serve-expired-ttl 0 |
| serve-expired-reply-ttl | 回应的过期缓存 TTL | 5 | 秒，0 表示停用超时，大于 0 表示指定的超时的秒数 | serve-expired-reply-ttl 30 |
| serve-expired-prefetch-time | 过期缓存预查询时间 | 28800 | 秒，到达对应超时时间后预查询时间 | serve-expired-prefetch-time 86400 |
| dualstack-ip-selection | 双栈 IP 优选 | yes | [yes\|no] | dualstack-ip-selection yes |
| dualstack-ip-selection-threshold | 双栈 IP 优选阈值 | 10ms | 单位为毫秒（ms） | dualstack-ip-selection-threshold [0-1000] |
| user | 进程运行用户 | root | user [username] | user nobody |
| ca-file | 证书文件 | /etc/ssl/<br />certs/ca-certificates.crt | 合法路径字符串 | ca-file /etc/ssl/certs/ca-certificates.crt |
| ca-path | 证书文件路径 | /etc/ssl/certs | 合法路径字符串 | ca-path /etc/ssl/certs |