// Copyright (c) 2009-2012 Bitcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "init.h" // for pwalletMain
#include "bitcoinrpc.h"
#include "ui_interface.h"
#include "base58.h"
#include <fstream>

#include <boost/lexical_cast.hpp>

#define printf OutputDebugStringF

using namespace json_spirit;
using namespace std;

class CTxDump
{
public:
    CBlockIndex *pindex;
    int64 nValue;
    bool fSpent;
    CWalletTx* ptx;
    int nOut;
    CTxDump(CWalletTx* ptx = NULL, int nOut = -1)
    {
        pindex = NULL;
        nValue = 0;
        fSpent = false;
        this->ptx = ptx;
        this->nOut = nOut;
    }
};

Value importprivkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 3)
        throw runtime_error(
            "importprivkey <bitcrystalprivkey> [label] [rescan=true]\n"
            "Adds a private key (as returned by dumpprivkey) to your wallet.");

    string strSecret = params[0].get_str();
    string strLabel = "";
    if (params.size() > 1)
        strLabel = params[1].get_str();

    // Whether to perform rescan after import
    bool fRescan = true;
    if (params.size() > 2)
        fRescan = params[2].get_bool();

    CBitcoinSecret vchSecret;
    bool fGood = vchSecret.SetString(strSecret);

    if (!fGood) throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");

    CKey key;
    bool fCompressed;
    CSecret secret = vchSecret.GetSecret(fCompressed);
    key.SetSecret(secret, fCompressed);
    CKeyID vchAddress = key.GetPubKey().GetID();
    {
        LOCK2(cs_main, pwalletMain->cs_wallet);

        pwalletMain->MarkDirty();
        pwalletMain->SetAddressBookName(vchAddress, strLabel);

        if (!pwalletMain->AddKey(key))
            throw JSONRPCError(RPC_WALLET_ERROR, "Error adding key to wallet");
	
        if (fRescan) {
            pwalletMain->ScanForWalletTransactions(pindexGenesisBlock, true);
            pwalletMain->ReacceptWalletTransactions();
        }
    }

    return Value::null;
}

Value dumpprivkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "dumpprivkey <bitcrystaladdress>\n"
            "Reveals the private key corresponding to <bitcrystaladdress>.");

    string strAddress = params[0].get_str();
    CBitcoinAddress address;
    if (!address.SetString(strAddress))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid BitCrystal address");
    CKeyID keyID;
    if (!address.GetKeyID(keyID))
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to a key");
    CSecret vchSecret;
    bool fCompressed;
    if (!pwalletMain->GetSecret(keyID, vchSecret, fCompressed))
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key for address " + strAddress + " is not known");
    return CBitcoinSecret(vchSecret, fCompressed).ToString();
}

void getrawtransactiondetails(string txid, my_rawtransactioninformation & my)
{
	Value ret;
	Array par;
	par.push_back(txid);
	par.push_back(1);
	my.size = 0;
	try {
		ret = getrawtransaction(par, false);
	} catch (runtime_error ex) {
		return;
	} catch (Object ex) {
		return;
	}
	
	Value val;
	int size;
	my_vin vin_s = { 0 };
	my_vout vout_s = { 0 };
	if (ret.type() == obj_type)
    {
		Object obj = ret.get_obj();
		val = find_value(obj, "hex");
		if(val.type() != null_type)
		{
			my.hex = val.get_str();
		}
		val = find_value(obj, "txid");
		if(val.type() != null_type)
		{
			my.txid = val.get_str();
		}
		val = find_value(obj, "version");
		if(val.type() != null_type)
		{
			my.version = val.get_int();
		}
		val = find_value(obj, "locktime");
		if(val.type() != null_type)
		{
			my.locktime = val.get_int();
		}
		val = find_value(obj, "vin");
		if(val.type() != null_type)
		{
			if(val.type() == array_type)
			{
				Array vin = val.get_array();
				size = vin.size();
				for(int i = 0; i < size; i++)
				{
					if(vin[i].type() != obj_type)
					{
						continue;
					}
					memset(&vin_s, 0, sizeof(vin_s));
					
					Object obj2 = vin[i].get_obj();
					
					val = find_value(obj2, "txid");
					if(val.type() != null_type)
					{
						vin_s.txid = val.get_str();
					}
					val = find_value(obj2, "vout");
					if(val.type() != null_type)
					{
						vin_s.vout = val.get_int();
					}
					val = find_value(obj2, "scriptSig");
					Object obj3;
					if(val.type() != null_type)
					{
						if(val.type() == obj_type)
						{
							obj3 = val.get_obj();
							vin_s.scriptSig.asm_ = find_value(obj3, "asm").get_str();
							vin_s.scriptSig.hex = find_value(obj3, "hex").get_str(); 							
						}
					}
					val = find_value(obj2, "sequence");
					if(val.type() != null_type)
					{
						vin_s.sequence = val.get_int64();
					}
					my.vin.push_back(vin_s);
				}
			}
		}
		val = find_value(obj, "vout");
		if(val.type() != null_type)
		{
			if(val.type() == array_type)
			{
				Array vout = val.get_array();
				size = vout.size();
				for(int i = 0; i < size; i++)
				{
					if(vout[i].type() != obj_type)
					{
						continue;
					}
					memset(&vout_s, 0, sizeof(vout_s));
					
					Object obj2 = vout[i].get_obj();
					
					val = find_value(obj2, "value");
					if(val.type() != null_type)
					{
						vout_s.value = val.get_real();
					}
					 
					val = find_value(obj2, "n");
					if(val.type() != null_type)
					{
						vout_s.n = val.get_int();
					}
					val = find_value(obj2, "scriptPubKey");
					Object obj3;
					if(val.type() != null_type)
					{
						if(val.type() == obj_type)
						{
							obj3 = val.get_obj();
							vout_s.scriptPubKey.asm_ = find_value(obj3, "asm").get_str();
							vout_s.scriptPubKey.hex = find_value(obj3, "hex").get_str();
							vout_s.scriptPubKey.reqSigs = find_value(obj3, "reqSigs").get_int();
							vout_s.scriptPubKey.type = find_value(obj3, "type").get_str();
							vout_s.scriptPubKey.addresses = find_value(obj3, "addresses").get_array(); 
						}
					}
					my.vout.push_back(vout_s);
				}
			}
		}
		val = find_value(obj, "blockhash");
		if(val.type() != null_type)
		{
			my.blockhash = val.get_str();
		}
		val = find_value(obj, "confirmations");
		if(val.type() != null_type)
		{
			my.confirmations = val.get_int();
		}
		val = find_value(obj, "time");
		if(val.type() != null_type)
		{
			my.time = val.get_int64();
		}
		val = find_value(obj, "blocktime");
		if(val.type() != null_type)
		{
			my.blockhash = val.get_int64();
		}
	}
	my.size = sizeof(my);
}

Value my_outputrawtransaction(const Array& params, bool fHelp)
{
	if (fHelp || params.size() != 1)
        throw runtime_error("fick die henne\n");
	my_rawtransactioninformation my;
	getrawtransactiondetails(params[0].get_str(), my);
	string cool;
	if(my.size == 0)
	{
		cool += "error in code";
		return cool;
	}
	cool += my.vout.at(0).scriptPubKey.hex;
	return cool;
}

/*Array mygetnewaddress()
{
	Array array;
	if (!pwalletMain->IsLocked())
        pwalletMain->TopUpKeyPool();
    // Generate a new key that is added to wallet
    CPubKey newKey;
    if (!pwalletMain->GetKeyFromPool(newKey, false))
        throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");
    CKeyID keyID = newKey.GetID();
	CBitcoinAddress address(keyID);
    CSecret vchSecret;
    bool fCompressed;
	string strAdress = address.ToString();
    if (!pwalletMain->GetSecret(keyID, vchSecret, fCompressed))
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key for address " + strAddress + " is not known");
    CBitcoinSecret cBitcoinSecret(vchSecret, fCompressed);
	CPubKey vchPubKey;
	bool fPubKeyCompressed;
	bool fPubKeyIsScript;
    pwalletMain->GetPubKey(keyID, vchPubKey);
	fPubKeyCompressed = vchPubKey.IsCompressed();
	fPubKeyIsScript = false;
	string pubKey = HexStr(vchPubKey.Raw());
	string privKey = cBitcoinSecret.ToString();
	string pubKeyCompressed = "false";
	string privKeyCompressed = "false";
	string isScript = "false";
	if(fPubKeyCompressed)
		pubKeyCompressed = "true";
	if(fCompressed)
		privKeyCompressed = "true";
	if(fPubKeyIsScript)
		isScript = "true";
	array.push_back(strAdress);
	array.push_back(pubKey);
	array.push_back(privKey);
	array.push_back(pubKeyCompressed);
	array.push_back(privKeyCompressed);
	array.push_back(isScript);
	return array;
}

Value myimportkey(const Array& params, bool fHelp)
{
	  if (fHelp || params.size() != 1)
        throw runtime_error(
            "mydumpprivkey <bitcrystaladdress>\n"
            "Reveals the private key corresponding to <bitcrystaladdress>.");

      if (!pwalletMain->IsLocked())
        pwalletMain->TopUpKeyPool();
	string file = params[0].get_str();
	string sfile1 = file;
	Array * keyFile;
	
	ifstream file1(sfile1, ios::in | ios::binary);
	file1.seekg(0, file1.end);
	int length = file1.tellg();
	file1.seekg(0, file1.beg);
	char array1[length];
    if(!file1.is_open()){
        throw JSONRPCError(RPC_WALLET_ERROR, "File must be close!");
        return 0;
    }

    while(!file1.eof()){
		file1.read((char*)&array1[0], length);
    }
	
	file1.close();
	
	keyFile = (Array*)&array1[0];
	return *keyFile;
}

Value mydumpprivandpubkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "mydumpprivkey <bitcrystaladdress>\n"
            "Reveals the private key corresponding to <bitcrystaladdress>.");

      if (!pwalletMain->IsLocked())
        pwalletMain->TopUpKeyPool();
	string file = params[0].get_str();
	string sfile1 = file + ".pub";
	string sfile2 = file + ".priv";
    // Generate a new key that is added to wallet
    Array address1 = mygetnewaddress();
	string strAddress1 = address1[0].get_str();
	string pubKey1 = address1[1].get_str();
	string privKey1 = address1[2].get_str();
	string pubKeyCompressed1 = address1[3].get_str();
	string privKeyCompressed1 = address1[4].get_str();
	string isScript1 = address1[5].get_str();
	Array publicKeyFile;
	publicKeyFile.push_back(strAddress1);
	publicKeyFile.push_back(pubKey1);
	publicKeyFile.push_back(pubKeyCompressed1);
	publicKeyFile.push_back(isScript1);
	Array privKeyFile;
	privKeyFile.push_back(strAddress1);
	privKeyFile.push_back(privKey1);
	privKeyFile.push_back(privKeyCompressed1);
	privKeyFile.push_back(isScript1);
	ofstream file1(sfile1, ios::out | ios::app | ios::binary);

    if(!file1.is_open()){
        throw JSONRPCError(RPC_WALLET_ERROR, "File must be close!");
    } else {
        file1.write((char*)&publicKeyFile, sizeof(publicKeyFile));
        file1.close();
    }
	
	ofstream file2(sfile2, ios::out | ios::app | ios::binary);

    if(!file2.is_open()){
        throw JSONRPCError(RPC_WALLET_ERROR, "File must be close!");
    } else {
        file2.write((char*)&privKeyFile, sizeof(privKeyFile));
        file2.close();
    }
	sfile1 += ", ";
	sfile1 += sfile2;
	return sfile1;
}

Value mycreatemultisigaddressoffiles(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 4)
        throw runtime_error(
            "mydumpprivkey <bitcrystaladdress>\n"
            "Reveals the private key corresponding to <bitcrystaladdress>.");

      if (!pwalletMain->IsLocked())
        pwalletMain->TopUpKeyPool();
	Array kFile1 = (Array)myimportkey(params[0], false);
	Array kFile2 = (Array)myimportkey(params[1], false);
	Array kFile3 = (Array)myimportkey(params[2], false);
	string sfile1 = params[3].get_str() + ".multisigaddress";
	string mSig = "";
	mSig += "[\"";
	mSig += kFile1[1].get_str();
	mSig += "\",\"";
	mSig += kFile2[1].get_str();
	mSig += "\",\"";
	mSig += kFile3[1].get_str();
	mSig += "\"]";
	Array my;
	my.push_back(3);
	my.push_back(mSig);
	CScript inner = _createmultisig(my);
    CScriptID innerID = inner.GetID();
    CBitcoinAddress address(innerID);
    string address = address.ToString()));
    string reedemScript = HexStr(inner.begin(), inner.end());
	ofstream file1(sfile1, ios::out | ios::app | ios::binary);

    if(!file1.is_open()){
        throw JSONRPCError(RPC_WALLET_ERROR, "File must be close!");
    } else {
        file1.write((char*)&inner, sizeof(inner));
        file1.close();
    }
	
	Array par;
	par.push_back("txid");
	par.push_back(1);
	Value ret = getrawtransaction(par, false);
	Object obj = (Object)ret;
	Array arr = find_value(obj, "vout").get_array();
	int size = arr.size();
	Object lastObject;
	int value;
	int n;
	string hex;
	string type;
	string cmp = "scripthash";
	for(int i = 0; i < size; i++)
	{
		lastObject = (Object)arr[i].get_obj();
		value = (int)find_value(lastObject, "value");
		n = (int)find_value(lastObject, "n");
		hex = (string)find_value(lastObject, "hex");
		type = (string)find_value(lastObject, "type");
		if(type.compare(cmp) == 0)
		{
			break;
		}
		if(i+1==size)
			throw JSONRPCError(RPC_WALLET_ERROR, "error!");
	}
	string rawtransaction = "'[{\"txid\":\"8f2427f2b9dbba0b80ab7f9ab9a7d6605c14f64b03aa04b73d880a7a03ade8aa\",\"vout\":1,\"scriptPubKey\":\"a914f55d81479219dced6dfe0eadfbfeb10daa0a3d8a87\",\"redeemScript\":\"5221025397ecf84a520f5ff9af4beaf43a0ee9da4ce787b91ab67d9863a7fed441355621039016b03bf64977d585061242033e38e5f023c5aba41145d497ef52f5582e583a52ae\"}]' '{\"mgFmgSZuubcR9RDZR7EypuZRRVXjZY22S8\":100}'";
	string signrawtransaction = "'01000000c1f20e5301aae8ad037a0a883db704aa034bf6145c60d6a7b99a7fab800bbadbb9f227248f0100000000ffffffff0100e1f505000000001976a914081906b7089eef2ae9411b7ad9e323891a49d74088ac00000000' '[{\"txid\":\"8f2427f2b9dbba0b80ab7f9ab9a7d6605c14f64b03aa04b73d880a7a03ade8aa\",\"vout\":1,\"scriptPubKey\":\"a914f55d81479219dced6dfe0eadfbfeb10daa0a3d8a87\",\"redeemScript\":\"5221025397ecf84a520f5ff9af4beaf43a0ee9da4ce787b91ab67d9863a7fed441355621039016b03bf64977d585061242033e38e5f023c5aba41145d497ef52f5582e583a52ae\"}]' '[\"cW3rrh7R2EiKryGDH5AWFpNkY75226DLqMZh6LyUbo5kdN5dMKYK\"]'";
}*/

/*Value myimportprivkey(const Array& params, bool fHelp)
{
	if (fHelp || params.size() != 2)
        throw runtime_error(
            "myimportprivkey <bitcrystaladdress>\n"
            "Reveals the private key corresponding to <bitcrystaladdress>.");

	CBitcoinSecret cBitcoinSecret;
	
	string file = params[0].get_str();
	string password = params[1].get_str();
	
	ifstream file2(file, ios::in | ios::binary);
	
    if(!file2.is_open()){
        throw JSONRPCError(RPC_WALLET_ERROR, "File must be close!");
        return 0;
    }


    while(!file2.eof()){
        file2.read((char *)&cBitcoinSecret, sizeof(cBitcoinSecret));
    }
}

Value mydumpprivkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "mydumpprivkey <bitcrystaladdress>\n"
            "Reveals the private key corresponding to <bitcrystaladdress>.");

      if (!pwalletMain->IsLocked())
        pwalletMain->TopUpKeyPool();
	string file = params[0].get_str();
	string password = params[1].get_str();
    // Generate a new key that is added to wallet
    CPubKey newKey;
	CKey cKey;
    if (!pwalletMain->GetKeyFromPool(newKey, false))
        throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");
    CKeyID keyID = newKey.GetID();
	CBitcoinAddress address(keyID);
    CSecret vchSecret;
    bool fCompressed;
	string strAdress = "Unknown";
    if (!pwalletMain->GetSecret(keyID, vchSecret, fCompressed))
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key for address " + strAddress + " is not known");
    CBitcoinSecret cBitcoinSecret(vchSecret, fCompressed);
	vector<unsigned char> myPassword;
	string addr=address.ToString();
	int lengthAddr = addr.size();
	int lengthPassword = password.size();
	if (lengthPassword<lengthAddr)
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key for address " + strAddress + " is not known");
	int length = 0;
	char * pass = password.c_str();
	char * addre = addr.c_str();
	for(i = 0; i < lengthPassword; i++)
	{
		if(i<lengthAddr)
			myPassword.push_back((unsigned char)&pass[i] ^ (unsigned char)&addre[i]);
			myPassword.push_back((unsigned char)&pass[i] & (unsigned char)&addre[i]);
			myPassword.push_back((unsigned char)&pass[i] | (unsigned char)&addre[i]);
		else
			myPassword.push_back((unsigned char)&pass[i]);
	}
	uint160 myPasswordHash = Hash160(myPassword);
	int myPasswordHashLength = sizeof(myPasswordHash);
	vector<unsigned char> myVec;
	/*for(int i = 0; i < sizeof(int); i++)
	{
		myVec.push_back((unsigned char)&myPasswordHashLength[i]);
	}
	for(int i = 0; i < 1; i++)
	{
		myVec.push_back((unsigned char)10);
		myVec.push_back((unsigned char)13);
		myVec.push_back((unsigned char)1);
		myVec.push_back((unsigned char)2);
		myVec.push_back((unsigned char)3);
		myVec.push_back((unsigned char)4);
		myVec.push_back((unsigned char)10);
		myVec.push_back((unsigned char)13);
	}
	for(int i = 0; i < myPasswordHashLength; i++)
	{
		myVec.push_back((unsigned char)myPasswordHash[i]);
	}
	for(int i = 0; i < 1; i++)
	{
		myVec.push_back((unsigned char)10);
		myVec.push_back((unsigned char)13);
		myVec.push_back((unsigned char)1);
		myVec.push_back((unsigned char)2);
		myVec.push_back((unsigned char)3);
		myVec.push_back((unsigned char)4);
		myVec.push_back((unsigned char)10);
		myVec.push_back((unsigned char)13);
	}*/
	/*
	length = sizeof(cBitcoinSecret);
	for(int i = 0; i < sizeof(int); i++)
	{
		myVec.push_back((unsigned char)&length[i]);
	}
	for(int i = 0; i < 1; i++)
	{
		myVec.push_back((unsigned char)10);
		myVec.push_back((unsigned char)13);
		myVec.push_back((unsigned char)1);
		myVec.push_back((unsigned char)2);
		myVec.push_back((unsigned char)3);
		myVec.push_back((unsigned char)4);
		myVec.push_back((unsigned char)10);
		myVec.push_back((unsigned char)13);
	}
	for(int i = 0; i < length; i++)
	{
		if(i < myPasswordHashLength)
		{
			myVec.push_back((unsigned char*)&cBitcoinSecret[i] ^ ((unsigned char*)&myPasswordHash[i] & ~(unsigned char*)&cBitcoinSecret[i]));
		} else {
			myVec.push_back((unsigned char*)&cBitcoinSecret[i]);
		}
	}
	for(int i = 0; i < 1; i++)
	{
		myVec.push_back((unsigned char)10);
		myVec.push_back((unsigned char)13);
		myVec.push_back((unsigned char)1);
		myVec.push_back((unsigned char)2);
		myVec.push_back((unsigned char)3);
		myVec.push_back((unsigned char)4);
		myVec.push_back((unsigned char)10);
		myVec.push_back((unsigned char)13);
	}
	uint160 hash = Hash160(myVec);
	int hashLength = sizeof(hash);
	for(int i = 0; i < sizeof(int); i++)
	{
		myVec.push_back((unsigned char*)&hashLength[i]);
	}
	for(int i = 0; i < 1; i++)
	{
		myVec.push_back((unsigned char)10);
		myVec.push_back((unsigned char)13);
		myVec.push_back((unsigned char)1);
		myVec.push_back((unsigned char)2);
		myVec.push_back((unsigned char)3);
		myVec.push_back((unsigned char)4);
		myVec.push_back((unsigned char)10);
		myVec.push_back((unsigned char)13);
	}
	for(int i = 0; i < hashLength; i++)
	{
		myVec.push_back((unsigned char*)&hash[i]);
	}
	for(int i = 0; i < 1; i++)
	{
		myVec.push_back((unsigned char)10);
		myVec.push_back((unsigned char)13);
		myVec.push_back((unsigned char)1);
		myVec.push_back((unsigned char)2);
		myVec.push_back((unsigned char)3);
		myVec.push_back((unsigned char)4);
		myVec.push_back((unsigned char)10);
		myVec.push_back((unsigned char)13);
	}
	length = sizeof(address);
	for(int i = 0; i < sizeof(int); i++)
	{
		myVec.push_back((unsigned char*)&length[i]);
	}
	for(int i = 0; i < 1; i++)
	{
		myVec.push_back((unsigned char)10);
		myVec.push_back((unsigned char)13);
		myVec.push_back((unsigned char)1);
		myVec.push_back((unsigned char)2);
		myVec.push_back((unsigned char)3);
		myVec.push_back((unsigned char)4);
		myVec.push_back((unsigned char)10);
		myVec.push_back((unsigned char)13);
	}
	for(int i = 0; i < length; i++)
	{
		if(i < myPasswordHashLength)
			myVec.push_back((unsigned char*)&address[i] ^ (unsigned char*)&myPasswordHash[i]);
		else
			myVec.push_back((unsigned char*)&address[i]);
	}
	for(int i = 0; i < 1; i++)
	{
		myVec.push_back((unsigned char)10);
		myVec.push_back((unsigned char)13);
		myVec.push_back((unsigned char)1);
		myVec.push_back((unsigned char)2);
		myVec.push_back((unsigned char)3);
		myVec.push_back((unsigned char)4);
		myVec.push_back((unsigned char)10);
		myVec.push_back((unsigned char)13);
	}
	uint160 hash = Hash160(myVec);
	hashLength = sizeof(hash);
	for(int i = 0; i < sizeof(int); i++)
	{
		myVec.push_back((unsigned char*)&hashLength[i]);
	}
	for(int i = 0; i < 1; i++)
	{
		myVec.push_back((unsigned char)10);
		myVec.push_back((unsigned char)13);
		myVec.push_back((unsigned char)1);
		myVec.push_back((unsigned char)2);
		myVec.push_back((unsigned char)3);
		myVec.push_back((unsigned char)4);
		myVec.push_back((unsigned char)10);
		myVec.push_back((unsigned char)13);
	}
	for(int i = 0; i < hashLength; i++)
	{
		myVec.push_back((unsigned char*)&hash[i]);
	}
	for(int i = 0; i < 1; i++)
	{
		myVec.push_back((unsigned char)10);
		myVec.push_back((unsigned char)13);
		myVec.push_back((unsigned char)1);
		myVec.push_back((unsigned char)2);
		myVec.push_back((unsigned char)3);
		myVec.push_back((unsigned char)4);
		myVec.push_back((unsigned char)10);
		myVec.push_back((unsigned char)13);
	}
	string ve(myVec.begin(), myVec.end());
	Array newParams;
	newParams.push_back(addr);
	newParams.push_back(ve);
	string ret = (string)signmessage(newParams, false);
	length = ret.size();
	char * rete = ret.c_str();
	for(int i = 0; i < sizeof(int); i++)
	{
		myVec.push_back((unsigned char*)&length[i]);
	}
	for(int i = 0; i < 1; i++)
	{
		myVec.push_back((unsigned char)10);
		myVec.push_back((unsigned char)13);
		myVec.push_back((unsigned char)1);
		myVec.push_back((unsigned char)2);
		myVec.push_back((unsigned char)3);
		myVec.push_back((unsigned char)4);
		myVec.push_back((unsigned char)10);
		myVec.push_back((unsigned char)13);
	}
	for(int i = 0; i < length; i++)
	{
		myVec.push_back((unsigned char*)&rete[i]);
	}
	for(int i = 0; i < 1; i++)
	{
		myVec.push_back((unsigned char)10);
		myVec.push_back((unsigned char)13);
		myVec.push_back((unsigned char)1);
		myVec.push_back((unsigned char)2);
		myVec.push_back((unsigned char)3);
		myVec.push_back((unsigned char)4);
		myVec.push_back((unsigned char)10);
		myVec.push_back((unsigned char)13);
	}
	hashLength=sizeof(myVec)+sizeof(int);
	for(int i = 0; i < sizeof(int); i++)
	{
		myVec.push_back((unsigned char*)&hashLength[i]);
	}
	ofstream file2(file, ios::out | ios::app | ios::binary);

    if(!file2.is_open()){
        throw JSONRPCError(RPC_WALLET_ERROR, "File must be close!");
    } else {
        file2.write((char*)&myVec, sizeof(myVec));
        file2.close();
    }
	return address.ToString();
}*/
