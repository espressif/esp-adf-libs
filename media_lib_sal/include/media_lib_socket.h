/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#ifndef MEDIA_LIB_SOCKET_H
#define MEDIA_LIB_SOCKET_H

#include "media_lib_socket_reg.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Wrapper for socket accept
 * @return
 *       - ESP_ERR_NOT_SUPPORTED  Wrapper function not registered
 *       - Others                 Socket accept fail
 */
int media_lib_socket_accept(int s, struct sockaddr *addr, socklen_t *addrlen);

/**
 * @brief  Wrapper for socket bind
 * @return
 *       - ESP_ERR_NOT_SUPPORTED  Wrapper function not registered
 *       - Others                 Socket bind fail
 */
int media_lib_socket_bind(int s, const struct sockaddr *name, socklen_t namelen);

/**
 * @brief  Wrapper for socket shutdown
 * @return
 *       - ESP_ERR_NOT_SUPPORTED  Wrapper function not registered
 *       - Others                 Socket shutdown fail
 */
int media_lib_socket_shutdown(int s, int how);

/**
 * @brief  Wrapper for socket close
 * @return
 *       - ESP_ERR_NOT_SUPPORTED  Wrapper function not registered
 *       - Others                 Socket close fail
 */
int media_lib_socket_close(int s);

/**
 * @brief  Wrapper for socket connect
 * @return
 *       - ESP_ERR_NOT_SUPPORTED  Wrapper function not registered
 *       - Others                 Socket connect fail
 */
int media_lib_socket_connect(int s, const struct sockaddr *name, socklen_t namelen);

/**
 * @brief  Wrapper for socket listen
 * @return
 *       - ESP_ERR_NOT_SUPPORTED  Wrapper function not registered
 *       - Others                 Listen fail
 */
int media_lib_socket_listen(int s, int backlog);

/**
 * @brief  Wrapper for socket recv
 * @return
 *       - Others  Returned by recvb wrapper function directly
 */
ssize_t media_lib_socket_recv(int s, void *mem, size_t len, int flags);

/**
 * @brief  Wrapper for socket read
 * @return
 *       - Others  Returned by read wrapper function directly
 */
ssize_t media_lib_socket_read(int s, void *mem, size_t len);

/**
 * @brief  Wrapper for socket readv
 * @return
 *       - Others  Returned by readv wrapper function directly
 */
ssize_t media_lib_socket_readv(int s, const struct iovec *iov, int iovcnt);

/**
 * @brief  Wrapper for socket recvfrom
 * @return
 *       - Others  Returned by recvfrom wrapper function directly
 */
ssize_t media_lib_socket_recvfrom(int s, void *mem, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);

/**
 * @brief  Wrapper for socket recvmsg
 * @return
 *       - Others  Returned by recvmsg wrapper function directly
 */
ssize_t media_lib_socket_recvmsg(int s, struct msghdr *message, int flags);

/**
 * @brief  Wrapper for socket send
 * @return
 *       - Others  Returned by send wrapper function directly
 */
ssize_t media_lib_socket_send(int s, const void *dataptr, size_t size, int flags);

/**
 * @brief  Wrapper for socket sendmsg
 * @return
 *       - Others  Returned by sendmsg wrapper function directly
 */
ssize_t media_lib_socket_sendmsg(int s, const struct msghdr *message, int flags);

/**
 * @brief  Wrapper for socket sendto
 * @return
 *       - Others  Returned by sendto wrapper function directly
 */
ssize_t media_lib_socket_sendto(int s, const void *dataptr, size_t size, int flags, const struct sockaddr *to, socklen_t tolen);

/**
 * @brief  Wrapper for socket open function
 * @return
 *       - Others  Returned by socket open wrapper function directly
 */
int media_lib_socket_open(int domain, int type, int protocol);

/**
 * @brief  Wrapper for socket write
 * @return
 *       - Others  Returned by socket write wrapper function directly
 */
ssize_t media_lib_socket_write(int s, const void *dataptr, size_t size);

/**
 * @brief  Wrapper for socket writev
 * @return
 *       - Others  Returned by writev wrapper function directly
 */
ssize_t media_lib_socket_writev(int s, const struct iovec *iov, int iovcnt);

/**
 * @brief  Wrapper for socket select
 * @return
 *       - Others  Returned by select wrapper function directly
 */
int media_lib_socket_select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, media_lib_timeval *timeout);

/**
 * @brief  Wrapper for socket ioctl
 * @return
 *       - Others  Returned by ioctl wrapper function directly
 */
int media_lib_socket_ioctl(int s, long cmd, void *argp);

/**
 * @brief  Wrapper for socket fcntl
 * @return
 *       - Others  Returned by fcntl wrapper function directly
 */
int media_lib_socket_fcntl(int s, int cmd, int val);

/**
 * @brief  Wrapper for inet_ntop
 * @return
 *       - NULL  Convert to string fail
 */
const char *media_lib_socket_inet_ntop(int af, const void *src, char *dst, socklen_t size);

/**
 * @brief  Wrapper for inet_pton
 * @return
 *       - Others  Returned by inet_pton wrapper function directly
 */
int media_lib_socket_inet_pton(int af, const char *src, void *dst);

/**
 * @brief  Wrapper for setsockopt
 * @return
 *       - Others  Returned by setsockopt wrapper function directly
 */
int media_lib_socket_setsockopt(int s, int level, int optname, const void *opval, socklen_t optlen);

/**
 * @brief  Wrapper for getsockopt
 * @return
 *       - Others  Returned by getsockopt wrapper function directly
 */
int media_lib_socket_getsockopt(int s, int level, int optname, void *opval, socklen_t *optlen);

/**
 * @brief  Wrapper for getsockname
 * @return
 *       - Others  Returned by getsockname wrapper function directly
 */
int media_lib_socket_getsockname(int s, struct sockaddr *name, socklen_t *namelen);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MEDIA_LIB_SOCKET_H */
