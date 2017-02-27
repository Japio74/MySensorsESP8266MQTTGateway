bool IsInitialized();
void ReadBrokerIPAddress(uint8_t* pIPAddress);
int ReadBrokerPort();
void ReadNetworkName(String& networkName);
void ReadNetworkPass(String& networkPass);
void WriteMagicByte();
void WriteBrokerIP(uint8_t *pIp);
void WriteBrokerPort(int16_t port);
void WriteNetworkName(String pNetworkname);
void WriteNetworkPass(String pNetworkPass);
void InitializeDefaultBrokerIPAddress(uint8_t* pIPAddress);
void InvalidateData();

