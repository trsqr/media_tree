/*
 * tda18250b_i2c.c
 *
 *  Created on: Jun 16, 2016
 *      Author: sky
 */

#include <linux/i2c.h>
#include <linux/regmap.h>
#include "dvb_frontend.h"


#include "tda18250b.c"

struct tda18250b_dev mdev;

struct tda18250b_dev* tda18250b_getdev(void){
	return &mdev;
}


static int tda18250b_set_params(struct dvb_frontend *fe)
{
	struct tda18250b_dev *dev = fe->tuner_priv;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	UInt32 puIF;

	printk(
			KERN_DEBUG " tda18250b_set_params delivery_system=%u modulation=%u frequency=%u bandwidth_hz=%u symbol_rate=%u inversion=%d stream_id=%d\n",
			c->delivery_system, c->modulation, c->frequency, c->bandwidth_hz,
			c->symbol_rate, c->inversion, c->stream_id);

	if(tda18250b_set_tuner(c->frequency,c->delivery_system,c->modulation)==TRUE){
		tmbslTDA18250A_GetIF(0,0,  &puIF);
		dev->if_frequency = puIF;
		return 0;
	}
	return -1;
}

static int tda18250b_get_if_frequency(struct dvb_frontend *fe, u32 *frequency)
{
	struct tda18250b_dev *dev = fe->tuner_priv;
	if(dev){
		*frequency = dev->if_frequency;
	}else{
		*frequency = 0;
	}
	printk("IF -> %u\n",*frequency);
	return 0;
}
static int tda18250b_init(struct dvb_frontend *fe)
{
	return tda18250B_Init(0);
}
static const struct dvb_tuner_ops tda18250b_tuner_ops = {
	.info = {
		.name           = "NXP TDA18250B",

		.frequency_min  =  42000000,
		.frequency_max  = 864000000,
		.frequency_step =      1000,
	},

	.set_params    = tda18250b_set_params,
	.get_if_frequency = tda18250b_get_if_frequency,
	.init = tda18250b_init,
};

static int tda18250b_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct tda18250b_config *cfg = client->dev.platform_data;
	struct dvb_frontend *fe = cfg->fe;
	struct tda18250b_dev *dev;
	int ret;
	unsigned int chip_id[3] = {0};
	char *version;
	static const struct regmap_config regmap_config = { .reg_bits = 8,
			.val_bits = 8, };
	ret = -1;
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (dev == NULL) {
		ret = -ENOMEM;
		dev_err(&client->dev, "kzalloc() failed\n");
		goto err;
	}

	memcpy(&dev->cfg, cfg, sizeof(struct tda18250b_config));
	dev->client = client;
	dev->regmap = devm_regmap_init_i2c(client, &regmap_config);
	if (IS_ERR(dev->regmap)) {
		ret = PTR_ERR(dev->regmap);
		goto err;
	}

	/* check if the tuner is there */
	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 1); /* open I2C-gate */

	regmap_read(dev->regmap, 0x00, (int*)&chip_id[0]);
	regmap_read(dev->regmap, 0x01, (int*)&chip_id[1]);
	regmap_read(dev->regmap, 0x02, (int*)&chip_id[2]);

	dev_info(&dev->client->dev, "chip_id=%02x:%02x:%02x\n", chip_id[0],chip_id[1],chip_id[2]);

	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0); /* close I2C-gate */


	switch (chip_id[0]) {
	case 0xc7:
		version = "M"; /* master */
		break;
	case 0x47:
		version = "S"; /* slave */
		break;
	default:
		ret = -ENODEV;
		goto err;
	}

	if((chip_id[0]== TDA18250A_CHIP_ID_0) && (chip_id[1] == TDA18250A_CHIP_ID_1)){
			switch(chip_id[2]){
			case TDA18250A_REVISION_0:dev_info(&dev->client->dev, "NXP TDA18250AHN/%s successfully identified\n",
					version);break;
			case TDA18250B_REVISION_0:dev_info(&dev->client->dev, "NXP TDA18250BHN/%s successfully identified\n",
					version);break;
			default:dev_info(&dev->client->dev, "NXP unknow version successfully identified\n");break;

			}
		}

	fe->tuner_priv = dev;
	memcpy(&fe->ops.tuner_ops, &tda18250b_tuner_ops,
			sizeof(struct dvb_tuner_ops));
	i2c_set_clientdata(client, dev);
	memcpy(&mdev, dev, sizeof(struct tda18250b_dev));
	return 0;
	err:
	dev_dbg(&client->dev, "failed=%d\n", ret);
	kfree(dev);
	return ret;
}


static int tda18250b_remove(struct i2c_client *client)
{
	struct tda18250b_dev *dev = i2c_get_clientdata(client);
	struct dvb_frontend *fe = dev->cfg.fe;

	dev_dbg(&client->dev, "\n");

	memset(&fe->ops.tuner_ops, 0, sizeof(struct dvb_tuner_ops));
	fe->tuner_priv = NULL;
	kfree(dev);

	return 0;
}

static const struct i2c_device_id tda18250b_id[] = {
	{"tda18250b_i2c", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, tda18250b_id);

static struct i2c_driver tda18250b_driver = {
	.driver = {
		.name	= "tda18250b_i2c",
	},
	.probe		= tda18250b_probe,
	.remove		= tda18250b_remove,
	.id_table	= tda18250b_id,
};

module_i2c_driver(tda18250b_driver);

MODULE_DESCRIPTION("NXP TDA18250BHN silicon tuner driver");
MODULE_AUTHOR("Antti Palosaari <crope@iki.fi>");
MODULE_LICENSE("GPL");
