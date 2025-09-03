#include <iio.h>
#include <stdio.h>


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
    struct iio_device *sdr;
    struct stream_cfg cfg;
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

    size_t sdr_buffer_capacity;

    void shutdown(void){
        // printf("* Destroying buffers\n");
        // if (rxbuf) { iio_buffer_destroy(rxbuf); }
        // if (txbuf) { iio_buffer_destroy(txbuf); }

        // printf("* Disabling streaming channels\n");
        // if (rx0_i) { iio_channel_disable(rx0_i); }
        // if (rx0_q) { iio_channel_disable(rx0_q); }
        // if (tx0_i) { iio_channel_disable(tx0_i); }
        // if (tx0_q) { iio_channel_disable(tx0_q); }

        // printf("* Destroying context\n");
        // if (ctx) { iio_context_destroy(ctx); }
        // exit(0);
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
    SDR(int device_num, iodev device_type, size_t sdr_buffer_capacity)
    :   sdr_buffer_capacity(sdr_buffer_capacity)
    {
        cfg.bw_hz = 2000000;
        cfg.fs_hz = 5000000;
        cfg.lo_hz = 2800000000;
        cfg.rfport = "A";

        scan_ctx = iio_create_scan_context("usb", 0);
        ssize_t ret =  iio_scan_context_get_info_list(scan_ctx, &info);

        strcpy(uri,iio_context_info_get_uri(info[device_num]));
        sdr_ctx = iio_create_context_from_uri(uri);

        iio_context_info_list_free(info);
        iio_scan_context_destroy(scan_ctx);

        get_ad9361_stream_dev(device_type, &sdr);
        cfg_ad9361_streaming_ch(&cfg, TX, 0);
        get_ad9361_stream_ch(TX, sdr, 0, &tx0_i);
        get_ad9361_stream_ch(TX, sdr, 1, &tx0_q);

        iio_channel_enable(tx0_i);
        iio_channel_enable(tx0_q);

        txbuf = iio_device_create_buffer(sdr, sdr_buffer_capacity, false);
    }    


    void send(complex16_vector& buf){
        char *p_dat, *p_end;
        ptrdiff_t p_inc;

        p_dat = (char *) iio_buffer_start(txbuf);
        p_end = (char *) iio_buffer_end(txbuf);
        p_inc = iio_buffer_step(txbuf);

        for (size_t i = 0; i < sdr_buffer_capacity && p_dat < p_end; i++) {
            ((int16_t*)p_dat)[0] = buf[i].imag()*1024; // I
            ((int16_t*)p_dat)[1] = buf[i].real()*1024; // Q
            p_dat += p_inc;
        }

        iio_buffer_push(txbuf);

    }

    

};