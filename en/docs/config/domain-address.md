---
hide:
  - toc
---

# Specify Domain Address

In addition to blocking ads, `address` can also be used to specify the IP address of a domain.

## Basic Configuration Method

1. Use the `address /domain/ip` option to specify the IP, such as:

    ```shell
    address /example.com/1.2.3.4
    ```

    In the `address` option:

    * `/domain/` uses suffix matching algorithm, including subdomains.
    * `ip`: can be an IPv6 or IPv4 address.

1. Specify IPv6

    ```shell
    address /example.com/::1
    ```

## Automatically Expand PTR Records Corresponding to address

If you want to expand the PTR record corresponding to the above `address`, you can use the `expand-ptr-from-address` switch to turn on automatic expansion. The `expand-ptr-from-address` parameter can be set repeatedly, and the parameter takes effect for the `address` set after it.

```shell
expand-ptr-from-address yes
address /example.com/1.2.3.4
expand-ptr-from-address no
```