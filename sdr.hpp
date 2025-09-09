#include <iio.h>
#include <stdio.h>
#include "parcer.hpp"


enum iodev { RX, TX };

#define IIO_ENSURE(expr) { \
	if (!(expr)) { \
		(void) fprintf(stderr, "assertion failed (%s:%d)\n", __FILE__, __LINE__); \
		(void) abort(); \
	} \
}


struct stream_cfg {
	long long bw_hz; // Analog banwidth in Hz
	long long fs_hz; // Baseband sample rate in Hz
	long long lo_hz; // Local oscillator frequency in Hz
	const char* rfport; // Port name
};


class SDR{

private:
    ConfigMap config;
    int16_t mult;

    struct iio_device *tx_sdr;
    struct iio_device *rx_sdr; 
    struct stream_cfg tx_cfg;
    struct stream_cfg rx_cfg;
    struct iio_scan_context *scan_ctx   = nullptr;
    struct iio_context_info **info      = nullptr;
    struct iio_context *sdr_ctx         = nullptr;

    char uri[20];
    char tmpstr[64];

    struct iio_channel *rx0_i = nullptr;
    struct iio_channel *rx0_q = nullptr;
    struct iio_channel *tx0_i = nullptr;
    struct iio_channel *tx0_q = nullptr;
    
    struct iio_buffer  *txbuf = nullptr;
    struct iio_buffer  *rxbuf = nullptr;

    size_t sdr_buffer_capacity;
    size_t rx_buf_size;


    void shutdown(void){

    }

    void errchk(int v, const char* what) {
        if (v < 0) { fprintf(stderr, "Error %d writing to channel \"%s\"\nvalue may not be supported.\n", v, what); shutdown(); }
    }

    void wr_ch_lli(struct iio_channel *chn, const char* what, long long val)
    {
        errchk(iio_channel_attr_write_longlong(chn, what, val), what);
    }

    void wr_ch_str(struct iio_channel *chn, const char* what, const char* str)
    {
        errchk(iio_channel_attr_write(chn, what, str), what);
    }

    char* get_ch_name(const char* type, int id)
    {
        snprintf(tmpstr, sizeof(tmpstr), "%s%d", type, id);
        return tmpstr;
    }

    struct iio_device* get_ad9361_phy(void)
    {
        struct iio_device *dev =  iio_context_find_device(sdr_ctx, "ad9361-phy");
        IIO_ENSURE(dev && "No ad9361-phy found");
        return dev;
    }

    bool get_ad9361_stream_dev(enum iodev d, struct iio_device **dev)
    {
        switch (d) {
        case TX: *dev = iio_context_find_device(sdr_ctx, "cf-ad9361-dds-core-lpc"); return *dev != NULL;
        case RX: *dev = iio_context_find_device(sdr_ctx, "cf-ad9361-lpc");  return *dev != NULL;
        default: IIO_ENSURE(0); return false;
        }
    }

    bool get_ad9361_stream_ch(enum iodev d, struct iio_device *dev, int chid, struct iio_channel **chn)
    {
        *chn = iio_device_find_channel(dev, get_ch_name("voltage", chid), d == TX);
        if (!*chn)
            *chn = iio_device_find_channel(dev, get_ch_name("altvoltage", chid), d == TX);
        return *chn != NULL;
    }

    bool get_phy_chan(enum iodev d, int chid, struct iio_channel **chn)
    {
        switch (d) {
        case RX: *chn = iio_device_find_channel(get_ad9361_phy(), get_ch_name("voltage", chid), false); return *chn != NULL;
        case TX: *chn = iio_device_find_channel(get_ad9361_phy(), get_ch_name("voltage", chid), true);  return *chn != NULL;
        default: IIO_ENSURE(0); return false;
        }
    }

    bool get_lo_chan(enum iodev d, struct iio_channel **chn)
    {
        switch (d) {
        case RX: *chn = iio_device_find_channel(get_ad9361_phy(), get_ch_name("altvoltage", 0), true); return *chn != NULL;
        case TX: *chn = iio_device_find_channel(get_ad9361_phy(), get_ch_name("altvoltage", 1), true); return *chn != NULL;
        default: IIO_ENSURE(0); return false;
        }
    }

    bool cfg_ad9361_streaming_ch(struct stream_cfg *cfg, enum iodev type, int chid)
    {
        struct iio_channel *chn = NULL;

        if (!get_phy_chan(type, chid, &chn)) {	return false; }
        wr_ch_str(chn, "rf_port_select",     cfg->rfport);
        wr_ch_lli(chn, "rf_bandwidth",       cfg->bw_hz);
        wr_ch_lli(chn, "sampling_frequency", cfg->fs_hz);


        if (!get_lo_chan(type, &chn)) { return false; }
        wr_ch_lli(chn, "frequency", cfg->lo_hz);
        return true;
    }

public:
    SDR(int device_num, size_t sdr_buffer_capacity, const std::string& CONFIGNAME)
    :   config(parse_config(CONFIGNAME)),
        mult(config["mult"]),
        sdr_buffer_capacity(sdr_buffer_capacity),
        rx_buf_size(sdr_buffer_capacity*config["rx_buf_size"])
    {

        tx_cfg.bw_hz = config["bw_hz"];
        tx_cfg.fs_hz = config["fs_hz"];
        tx_cfg.lo_hz = config["lo_hz"];
        tx_cfg.rfport = "A";

        rx_cfg.bw_hz = config["bw_hz"];
        rx_cfg.fs_hz = config["fs_hz"];
        rx_cfg.lo_hz = config["lo_hz"];
        rx_cfg.rfport = "A_BALANCED";

        scan_ctx = iio_create_scan_context("usb", 0);
        ssize_t ret =  iio_scan_context_get_info_list(scan_ctx, &info);

        strcpy(uri,iio_context_info_get_uri(info[device_num]));
        sdr_ctx = iio_create_context_from_uri(uri);

        iio_context_info_list_free(info);
        iio_scan_context_destroy(scan_ctx);

        get_ad9361_stream_dev(TX, &tx_sdr);
        cfg_ad9361_streaming_ch(&tx_cfg, TX, 0);
        get_ad9361_stream_ch(TX, tx_sdr, 0, &tx0_i);
        get_ad9361_stream_ch(TX, tx_sdr, 1, &tx0_q);

        iio_channel_enable(tx0_i);
        iio_channel_enable(tx0_q);

        txbuf = iio_device_create_buffer(tx_sdr, sdr_buffer_capacity, false);

        get_ad9361_stream_dev(RX, &rx_sdr);
        cfg_ad9361_streaming_ch(&rx_cfg, RX, 0);

        char hardwaregain[16];
        snprintf(hardwaregain, 16, "%.6lf dB", (double)config["hardwaregain"]);

        struct iio_device* phy = iio_context_find_device(sdr_ctx, "ad9361-phy");
        struct iio_channel* phy_voltage0 = iio_device_find_channel(phy, "voltage0", false);
        iio_channel_enable(phy_voltage0);
        iio_channel_attr_write(phy_voltage0, "gain_control_mode", "manual");
        iio_channel_attr_write(phy_voltage0, "hardwaregain", hardwaregain);
        iio_channel_disable(phy_voltage0);

        get_ad9361_stream_ch(RX, rx_sdr, 0, &rx0_i);
        get_ad9361_stream_ch(RX, rx_sdr, 1, &rx0_q);

        iio_channel_enable(rx0_i);
        iio_channel_enable(rx0_q);

        rxbuf = iio_device_create_buffer(rx_sdr, rx_buf_size, false);
    }    


    void send(complex16_vector& buf){
        char *p_dat, *p_end;
        ptrdiff_t p_inc;

        p_dat = (char *) iio_buffer_start(txbuf);
        p_end = (char *) iio_buffer_end(txbuf);
        p_inc = iio_buffer_step(txbuf);

        for (size_t i = 0; i < sdr_buffer_capacity && p_dat < p_end; i++) {
            ((int16_t*)p_dat)[0] = buf[i].imag()*16;
            ((int16_t*)p_dat)[1] = buf[i].real()*16;
            p_dat += p_inc;
        }

        iio_buffer_push(txbuf);

    }


void recv(complex16_vector& buf) {

    ssize_t ret = iio_buffer_refill(rxbuf);

    buf.resize(rx_buf_size);
    char *p_dat, *p_end;
    ptrdiff_t p_inc;


    p_dat = (char *) iio_buffer_start(rxbuf);
    p_end = (char *) iio_buffer_end(rxbuf);
    p_inc = iio_buffer_step(rxbuf);

    size_t i = 0;
    while (p_dat < p_end && i < buf.size()) {
        buf[i] = std::complex<int16_t>(((int16_t*)p_dat)[0], ((int16_t*)p_dat)[1]);

        p_dat += p_inc;
        i++;
    }
}

    

};