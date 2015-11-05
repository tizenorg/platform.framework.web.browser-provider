/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <browser-provider.h>
#include <browser-provider-log.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <glib.h>
#include <sync-adaptor.h>
#include <sync-adaptor-cloud.h>

static char *_device_id = NULL;

static int is_login = SYNC_LOGOUT;

static void int_to_char(int64_t num, char* string)
{
    int m=1;
	int index=0;
	char str[20]={0,};
	if(num == 0)
	{
		*string = '0';
		return;
	}
    while(num>0)
    {
        *(str+index)=num%10+'0';
        index++;
        num=num/10;
    }
	index--;
	m = index;
	while(m>=0)
	{
		*(string+m) = *(str+(index-m));
		m--;
	}
}

static void conv_mac( const char *data, char *cvrt_str, int sz )
{
	static char buf[128] = {0x00,};
	char t_buf[8];
	char *stp = strtok( (char *)data , ":" );
	int temp=0;
	do
	{
		memset( t_buf, 0x0, sizeof(t_buf) );
		sscanf( stp, "%x", &temp );
		snprintf( t_buf, sizeof(t_buf)-1, "%02X", temp );
		strncat( buf, t_buf, sizeof(buf)-1 );
		strncat( buf, ":", sizeof(buf)-1 );
	} while( (stp = strtok( NULL , ":" )) != NULL );
	buf[strlen(buf) -1] = '\0';
	strncpy( cvrt_str, buf, sz );
}

static char *get_mac()
{
	int r, iter;
	int sk;
	struct ifreq ifr;
	char *mac;
	char *interface_string[2];
	interface_string[0] = "eth0";
	interface_string[1] = "wlan0";
	sk = socket(AF_INET, SOCK_STREAM, 0);
	if (sk == -1) {
		TRACE_ERROR("socket() failed(%s)", strerror(errno));
		return NULL;
	}
	for (iter = 0; iter<2; iter++)
	{
		strcpy(ifr.ifr_name, interface_string[iter]);
		r = ioctl(sk, SIOCGIFHWADDR, &ifr);

		if (r==0)
		{
			TRACE_INFO("interface %s found", interface_string[iter]);
			break;
		}
	}
	close(sk);
	if (r < 0) {
		TRACE_ERROR("ioctl failed(%d)", r);
		return NULL;
	}
	mac = (char*)calloc(20, sizeof(char));
	memset(mac, 0x00,20);
	if (!mac) {
		TRACE_ERROR("calloc() failed");
		return NULL;
	}
	conv_mac(ether_ntoa((struct ether_addr *)(ifr.ifr_hwaddr.sa_data)), mac, 20);
	TRACE_INFO("mac [%s]", mac);
	return mac;
}

static char *_get_device_info(void)
{
	char* dev_info = NULL;
	dev_info = get_mac();
	return dev_info;
}

int bp_sync_is_login(void)
{
	if (is_login == SYNC_LOGIN)
		return 1;
	else
		return 0;
}

int bp_sync_login(char* guid, char* accesstoken)
{
	int ret = 0;
	TRACE_DEBUG("guid:%s ,accesstoken:%s",guid,accesstoken);

	ret = cloud_login(guid, accesstoken);
	if(ret != 0){
		TRACE_DEBUG("Error!");
		is_login = SYNC_LOGOUT;
		return -1;
	}
	is_login = SYNC_LOGIN;

	return 0;
}

int bp_sync_logout()
{
	is_login = 0;

	return 0;
}

int sync_init(char* app_id, char* app_package_name)
{
	int ret=-1;
	_device_id = _get_device_info();
	TRACE_DEBUG("Device ID %s",_device_id);
    ret = cloud_init(_device_id, app_id, app_package_name);

	if (ret != 0) {
		TRACE_ERROR("cloud init failed %d", ret);
		return -1;
	}else{
		return 0;
	}
}

char* get_device_id(){
	return _device_id;
}

