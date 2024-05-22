#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <poll.h>
#include <stdint.h>
#include <sys/types.h>

/* DATA */

// is set by the library to 1 when SSL is available, 0 otherwise
#ifndef SSL_AVAILABLE
#define SSL_AVAILABLE 0
#endif

#define MAX_CONNECTIONS 4096 // maximum number of pending connections
#define TIMEOUT_DEFAULT 1000 // default timeout for recv_all_data
#define TIMEOUT_INFINITE -1  // infinite timeout for recv_all_data

// TODO: err_type syntax is ugly, need to find an alternative
enum err_type {
    SYS,    // system error
    GAI,    // getaddrinfo error
    SOCK,   // socket error
    BIND,   // bind error
    LISTEN, // listen error
    CONN,   // connect error
};

enum io_info_type {
    FILE_IO,     // file i/o, used for reading and writing files
    ACCEPT_IO,   // accept i/o, used for accepting network connections
    CONNECTED_IO // connect i/o, an existing network connection
};

typedef struct io_info io_info_t;
typedef struct ssl_loader ssl_loader_t;

struct pollio {
    io_info_t *io_info; // i/o info
    short events;       // requested events
    short revents;      // returned events
};

/**
 * @brief Packet header.
 *
 * @param header_len - the length of the header
 * @param data_len - the length of the data
 * @param data_type - the user defined data type flag
 */
struct pkt_hdr {
    uint32_t header_len; // length of the header
    uint32_t data_len;   // length of the data
    uint32_t data_type;  // user defined data type flag
};

/**
 * @brief Packet.
 *
 * @param hdr - the packet header
 * @param data - the packet data
 */
struct packet {
    struct pkt_hdr *hdr; // packet header
    void *data;          // packet data
};

/* FUNCTIONS */

/**
 * @brief Create a new io_info object.
 *
 * The type of the object will determine the type of i/o that can be performed.
 * The file descriptor will be stored in the io_info object. Note that in this
 * case, the file descriptor will by default not be closed when the io_info
 * object is freed, unlike when the object is created from one of the other
 * dedicated functions.
 *
 * @param fd - The file descriptor.
 * @param type - The type of i/o.
 * @param err - Where to store the error code.
 * @return io_info_t* - The io_info object.
 */
io_info_t *new_io_info(int fd, int type, int *err);

/**
 * @brief Create a new io_info object for file operations.
 *
 * The underlying file descriptor will be closed when the io_info object is
 * freed.
 *
 * @param filename - The name of the file.
 * @param flags - The flags for opening the file.
 * @param mode - The mode for opening the file.
 * @param err - Where to store the error code.
 * @return io_info_t* - The io_info object.
 */
io_info_t *new_file_io_info(const char *filename, int flags, mode_t mode,
                            int *err);

/**
 * @brief Create a new io_info object for accepting network connections.
 *
 * Any underlying socket will be closed when the io_info object is freed.
 *
 * @param port - The port to listen on.
 * @param err - Where to store the error code.
 * @param err_type - Where to store the error type.
 * @return io_info_t* - The io_info object.
 */
io_info_t *new_accept_io_info(const char *port, int *err, int *err_type);

/**
 * @brief Create a new io_info object for connecting to remote hosts.
 *
 * Any underlying socket will be closed when the io_info object is freed.
 *
 * @param host - The remote host to connect to.
 * @param port - The remote port to connect on.
 * @param err - Where to store the error code.
 * @param err_type - Where to store the error type.
 * @return io_info_t* - The io_info object.
 */
io_info_t *new_connect_io_info(const char *host, const char *port, int *err,
                               int *err_type);

/**
 * @brief Create a new SSL loader.
 *
 * The SSL loader will store any stateful SSL information needed for use with
 * the io_info objects, allowing multiple objects to use the same
 * configurations.
 *
 * @param err - Where to store the error code.
 * @return ssl_loader_t* - The SSL loader.
 */
ssl_loader_t *new_ssl_loader(int *err);

/**
 * @brief Free an io_info object.
 *
 * @param io_info - The io_info object to free.
 */
void free_io_info(io_info_t *io_info);

/**
 * @brief Free an SSL loader.
 *
 * @param loader - The SSL loader to free.
 */
void free_ssl_loader(ssl_loader_t *loader);

/**
 * @brief Get the file descriptor from an io_info object.
 *
 * Note that the file descriptor is not duplicated, so it should not be closed
 * until the io_info object is freed.
 *
 * @param io_info - The io_info object.
 * @param type - Where to store the type of i/o. Can be NULL.
 * @return int - The file descriptor.
 */
int io_info_fd(io_info_t *io_info, int *type);

/**
 * @brief Get the host address from an io_info object.
 *
 * For accept i/o objects, this will be the address of the server. For connect
 * i/o objects, this will be the address of the client. For file i/o objects,
 * this will return NULL.
 *
 * @param io_info - The io_info object.
 * @return const char* - The host address.
 */
const char *io_info_host(io_info_t *io_info);

/**
 * @brief Get the service from an io_info object.
 *
 * For accept i/o objects, this will be the service of the server. For connect
 * i/o objects, this will be the service of the client. For file i/o objects,
 * this will return NULL.
 *
 * @param io_info - The io_info object.
 * @return const char* - The service.
 */
const char *io_info_serv(io_info_t *io_info);

/**
 * @brief Add SSL to an io_info object.
 *
 * Enables SSL on the io_info object. Not supported for file i/o objects.
 *
 * Possible errors:
 * - ENOTSUP: SSL is not available, or the io_info object is a file i/o object.
 * - EINVAL: The io_info object is NULL.
 *
 * @param io_info - The io_info object.
 * @param loader - The SSL loader.
 * @return int - 0 on success, non-zero on failure.
 */
int io_info_add_ssl(io_info_t *io_info, ssl_loader_t *loader);

/**
 * @brief Wrapper for poll(2) that uses io_info objects.
 *
 * The function will return the return value of poll(2) and will set the revents
 * field of the io_info objects.
 *
 * In the event of an error, the function will return a negative value,
 * corresponding to the inverse of what poll(2) would set errno to. i.e if
 * poll(2) would set errno to EAGAIN, the function will return -EAGAIN.
 *
 * @param ios - The array of io_info objects.
 * @param nfds - The number of io_info objects.
 * @param timeout - The timeout in milliseconds.
 * @return int - The return value of poll(2).
 */
int poll_io_info(struct pollio *ios, nfds_t nfds, int timeout);

/**
 * @brief Accept a new connection on a socket io_info.
 *
 * @param io_info - The io_info object to accept on.
 * @param err - Where to store the error code.
 * @return io_info_t* - The new io_info object.
 */
io_info_t *io_accept(io_info_t *io_info, int *err);

/**
 * @brief Read a fixed amount of data from an io_info object.
 *
 * If an error occurs, a non-zero error code will be returned
 * Possible errors:
 *   ENODATA: - the file descriptor has reached the end of the file early
 * See read(2) for additional possible errors.
 *
 * @param io_info - the io_info object
 * @param buff - pointer to store the data
 * @param read_sz - the size of the data to read
 * @return int - 0 on success, non-zero on failure
 */
int read_exact(io_info_t *io_info, void *buff, size_t read_sz);

/**
 * @brief Write all data to an io_info object.
 *
 * The function handles partial writes and will continue to write until all the
 * data is written.
 *
 * If an error occurs, a non-zero error code will be returned
 * See write(2) for possible errors.
 *
 * @param io_info - the io_info object
 * @param buf - the data to send
 * @param len - the length of the data
 * @return int - 0 on success, non-zero on failure
 */
int write_all(io_info_t *io_info, const void *buf, size_t len);

/**
 * @brief Free a packet.
 *
 * Frees the memory allocated for the packet.
 *
 * @param pkt - the packet to free
 */
void free_packet(struct packet *pkt);

/**
 * @brief Write packet data to an io_info object.
 *
 * The data will be sent in a packet with a header containing the length of the
 * serialized data. recv_pkt_data or read_pkt should be used to read the data
 * in the proper format. The data_type flag is used to specify the type of the
 * data. The data_type flag is not checked here and should be checked by the
 * application.
 *
 * Possible errors are the same as write(2).
 *
 * @param io_info - the io_info object
 * @param data - the data to write
 * @param len - the length of the data
 * @param data_type - flag for the type of the data
 * @return int - 0 on success, non-zero on failure
 */
int write_pkt_data(io_info_t *io_info, void *data, size_t len,
                   uint32_t data_type);

/**
 * @brief Receive packet data from an io_info object.
 *
 * Receives packet data from the given file descriptor and returns a pointer to
 * a packet struct containing the header and data. The other end must use
 * write_pkt_data to send the data, or the read will fail.
 *
 * Possible errors:
 *      ENODATA: The socket has reached the end of the file early.
 *      EINVAL: Invalid value in reported header length
 *      ENOMEM: Out of memory.
 * See read(2) for more details.
 *
 * @param io_info - the io_info object
 * @param err - the error code
 * @return struct packet* - pointer to the packet on success, NULL on failure
 */
struct packet *read_pkt(io_info_t *io_info, int *err);

/**
 * @brief Receive packet data from an io_info object.
 *
 * Receives packet data from the given socket and returns a pointer to a packet
 * struct containing the header and data. The other end must use write_pkt_data
 * to send the data, or the read will fail.
 *
 * The only difference between this function and read_pkt is that this function
 * will block waiting on available data, with an optional timeout.
 *
 * Possible errors:
 *      ETIMEDOUT: The operation timed out.
 *      ENODATA: The socket has reached the end of the file early.
 *      EINVAL: Invalid value in reported header length
 *      ENOMEM: Out of memory.
 * See poll(2), read(2) for more details.
 *
 * @param io_info - the io_info object
 * @param timeout - the timeout in milliseconds
 * @param err - the error code
 * @return struct packet* - pointer to the packet on success, NULL on failure
 */
struct packet *recv_pkt_data(io_info_t *io_info, int timeout, int *err);

#endif /* SERIALIZATION_H */
