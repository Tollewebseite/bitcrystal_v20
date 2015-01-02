// Copyright (c) 2009-2012 Bitcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "init.h" // for pwalletMain
#include "bitcoinrpc.h"
#include "ui_interface.h"
#include "base58.h"
#include "convert_functions.h"
#include <fstream>

#include <boost/lexical_cast.hpp>

#define printf OutputDebugStringF

using namespace json_spirit;
using namespace std;
using namespace my_convert_functions;

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

bool GetPrivKey(std::string & address, std::string & privKey)
{
	Value ret;
	Array par;
	par.push_back(address);
	bool allok=true;
	try {
		ret = dumpprivkey(par, false);
	} catch (runtime_error ex) {
		allok=false;
		return false;
	} catch (Object ex) {
		allok=false;
		return false;
	}
	allok=ret.type() == str_type;
	if(allok)
	{
		privKey=ret.get_str();
	}
	return allok;
}

bool hasPrivKey(std::string & address)
{
	Value ret;
	Array par;
	par.push_back(address);
	bool allok=true;
	try {
		ret = dumpprivkey(par, false);
	} catch (runtime_error ex) {
		allok=false;
		return false;
	} catch (Object ex) {
		allok=false;
		return false;
	}
	allok=ret.type() == str_type;
	return allok;
}

bool GetPubKey(std::string & address, std::string & pubKey)
{
	Value ret;
	Array par;
	par.push_back(address);
	bool allok=true;
	try {
		ret = validateaddress(par, false);
	} catch (runtime_error ex) {
		allok=false;
		return allok;
	} catch (Object ex) {
		allok=false;
		return allok;
	}
	allok=ret.type() == obj_type;
	if(allok)
	{
		Object obj = ret.get_obj();
		ret = find_value(obj, "pubkey");
		allok = ret.type() == str_type;
		if(allok)
		{
			pubKey=ret.get_str();
		}
	}
	return allok;
}

bool hasPubKey(std::string & address)
{
	Value ret;
	Array par;
	par.push_back(address);
	bool allok=true;
	try {
		ret = validateaddress(par, false);
	} catch (runtime_error ex) {
		allok=false;
		return allok;
	} catch (Object ex) {
		allok=false;
		return allok;
	}
	allok=ret.type() == obj_type;
	if(allok)
	{
		Object obj = ret.get_obj();
		ret = find_value(obj, "pubkey");
		allok = ret.type() == str_type;
	}
	return allok;
}

bool isMultisigAddress(std::string & address)
{
	bool allok=false;
	allok=hasRedeemScript(address);
	return allok;
}

bool GetBitcoinAddressOfPubKey(string & pubKey, string & address)
{
	if(IsHex(pubKey))
	{
		CPubKey vchPubKey(ParseHex(pubKey));
        if (!vchPubKey.IsValid())
		{
			address="";
			return false;
		}
		
		address=CBitcoinAddress(vchPubKey.GetID()).ToString();
		return true;
	} else {
		address="";
		return false;
	}
}
bool IsValidPubKey(string & pubKey)
{
	if(IsHex(pubKey))
	{
		CPubKey vchPubKey(ParseHex(pubKey));
        if (!vchPubKey.IsValid())
		{
			return false;
		}
		return true;
	} else {
		return false;
	}
}

bool IsMineBitcoinAddress(string & address)
{
	Value ret;
	Array par;
	par.push_back(address);
	bool allok=false;
	try {
		ret = validateaddress(par, false);
	} catch (runtime_error ex) {
		allok=false;
		return allok;
	} catch (Object ex) {
		allok=false;
		return allok;
	}
	allok=ret.type() == obj_type;
	if(allok)
	{
		Object obj = ret.get_obj();
		ret = find_value(obj, "ismine");
		allok = ret.type() == bool_type;
		if(allok)
		{
			allok=ret.get_bool();
		}
	}
	return allok;
}

bool IsMinePubKey(string & pubKey)
{
	string address;
	bool allok = GetBitcoinAddressOfPubKey(pubKey,address);
	if(allok)
		allok=IsMineBitcoinAddress(address);
	return allok;
}

Value getpubkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getpubkey\n"
            "Returns true or false.");

	string x = params[0].get_str();
	string pubKey;
	bool allok=GetPubKey(x,pubKey);
	if(!allok)
	{
		pubKey="";
	}
    return pubKey;
}

Value haspubkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "haspubkey\n"
            "Returns true or false.");

	string x = params[0].get_str();
    return hasPubKey(x);
}

Value getprivkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getprivkey\n"
            "Returns true or false.");

	string x = params[0].get_str();
	string privKey;
	bool allok = GetPrivKey(x,privKey);
	if(!allok)
	{
		privKey="";
	}
	return privKey;
}

Value hasprivkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "hasprivkey\n"
            "Returns true or false.");

	string x = params[0].get_str();
    return hasPrivKey(x);
}

Value ismultisigaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "ismultisigaddress\n"
            "Returns true or false.");

	string x = params[0].get_str();
    return isMultisigAddress(x);
}

Value getbitcoinaddressofpubkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getbitcoinaddressofpubkey\n"
            "Returns true or false.");

	string x = params[0].get_str();
	string address;
    bool allok = GetBitcoinAddressOfPubKey(x, address);
	if(!allok)
		address="";
	return address;
}

Value isvalidpubkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "isvalidpubkey\n"
            "Returns true or false.");

	string x = params[0].get_str();
    bool allok = IsValidPubKey(x);
	return allok;
}

Value isminebitcoinaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "isminebitcoinaddress\n"
            "Returns true or false.");

	string x = params[0].get_str();
    bool allok = IsMineBitcoinAddress(x);
	return allok;
}

Value isminepubkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "isminepubkey\n"
            "Returns true or false.");

	string x = params[0].get_str();
    bool allok = IsMinePubKey(x);
	return allok;
}

bool getrawtransactiondetails(std::string & txid, my_rawtransactioninformation & my)
{
	Value ret;
	Array par;
	par.push_back(txid);
	par.push_back(1);
	bool allok=true;
	try {
		ret = getrawtransaction(par, false);
	} catch (runtime_error ex) {
		allok=false;
		return allok;
	} catch (Object ex) {
		allok=false;
		return allok;
	}
	
	Value val;
	int size;
	my_vin vin_s;
	my_vout vout_s;
	try{
	if (ret.type() == obj_type)
    {
		//outs += ",hallo-1";
		Object obj = ret.get_obj();
		val = find_value(obj, "hex");
		if(val.type() != null_type)
		{
			my.hex = val.get_str();
			//outs += ",hallo-2";
		}
		val = find_value(obj, "txid");
		if(val.type() != null_type)
		{
			my.txid = val.get_str();
			//outs += ",hallo-3";
		}
		val = find_value(obj, "version");
		if(val.type() != null_type)
		{
			my.version = val.get_int();
			//outs += ",hallo-4";
		}
		val = find_value(obj, "locktime");
		if(val.type() != null_type)
		{
			my.locktime = val.get_int();
			//outs += ",hallo-5";
		}
		val = find_value(obj, "vin");
		if(val.type() != null_type)
		{
			if(val.type() == array_type)
			{
				Array vin = val.get_array();
				size = vin.size();
				//outs += ",hallo-6";
				for(int i = 0; i < size; i++)
				{
					if(vin[i].type() != obj_type)
					{
						continue;
					}
					//outs += ",hallo-7";
					vin_s.clear();
					
					Object obj2 = vin[i].get_obj();
					
					val = find_value(obj2, "txid");
					if(val.type() != null_type)
					{
						vin_s.txid = val.get_str();
						//outs += ",hallo-8";
					}
					val = find_value(obj2, "vout");
					if(val.type() != null_type)
					{
						vin_s.vout = val.get_int();
						//outs += ",hallo-9";
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
							//outs += ",hallo-10";
						}
						//outs += ",hallo-11";
					}
					val = find_value(obj2, "sequence");
					if(val.type() != null_type)
					{
						vin_s.sequence = val.get_int64();
						//outs += ",hallo-12";
					}
					my.vin.push_back(vin_s);
					//outs += ",hallo-13";
				}
				//outs += ",hallo-14";
			}
			//outs += ",hallo-16";
		}
		//outs += ",hallo-17";
		val = find_value(obj, "vout");
		if(val.type() != null_type)
		{
			//outs += ",hallo-18";
			if(val.type() == array_type)
			{
				//outs += ",hallo-19";
				Array vout = val.get_array();
				size = vout.size();
				for(int i = 0; i < size; i++)
				{
					//outs += ",hallo-20";
					if(vout[i].type() != obj_type)
					{
						continue;
					}
					//outs += ",hallo-21";
					vout_s.clear();
					
					Object obj2 = vout[i].get_obj();
					
					val = find_value(obj2, "value");
					if(val.type() != null_type)
					{
						//outs += ",hallo-22";
						vout_s.value = val.get_real();
					}
					 
					val = find_value(obj2, "n");
					if(val.type() != null_type)
					{
						//outs += ",hallo-23";
						vout_s.n = val.get_int();
					}
					val = find_value(obj2, "scriptPubKey");
					Object obj3;
					if(val.type() != null_type)
					{
						//outs += ",hallo-24";
						if(val.type() == obj_type)
						{
							//outs += ",hallo-25";
							obj3 = val.get_obj();
							vout_s.scriptPubKey.asm_ = find_value(obj3, "asm").get_str();
							vout_s.scriptPubKey.hex = find_value(obj3, "hex").get_str();
							vout_s.scriptPubKey.reqSigs = find_value(obj3, "reqSigs").get_int();
							vout_s.scriptPubKey.type = find_value(obj3, "type").get_str();
							vout_s.scriptPubKey.addresses = find_value(obj3, "addresses").get_array(); 
						}
					}
					//outs += ",hallo-26";
					my.vout.push_back(vout_s);
				}
			}
		}
		val = find_value(obj, "blockhash");
		if(val.type() != null_type)
		{
			//outs += ",hallo-27";
			my.blockhash = val.get_str();
		}
		val = find_value(obj, "confirmations");
		if(val.type() != null_type)
		{
			//outs += ",hallo-28";
			my.confirmations = val.get_int();
		}
		val = find_value(obj, "time");
		if(val.type() != null_type)
		{
			//outs += ",hallo-29";
			my.time = val.get_int64();
			//outs+=",";
			//outs+=get_string_of(my.time);
		}
		val = find_value(obj, "blocktime");
		if(val.type() != null_type)
		{
			//outs += ",hallo-30";
			my.blocktime = val.get_int64();
			//outs+=",";
			//outs+=get_string_of(my.blocktime);
		}
	}
	my.empty=false;
	} catch (runtime_error ex) {
		allok=false;
		return allok;
	} catch (Object ex) {
		allok=false;
		return allok;
	}
	return allok;
}

Value my_outputrawtransaction(const Array& params, bool fHelp)
{
	if (fHelp || params.size() != 1)
        throw runtime_error("fick die henne\n");
	my_rawtransactioninformation my;
	string x = "";
	x+=params[0].get_str();
	bool allok = getrawtransactiondetails(x, my);
	string t;
	if(my.empty || !allok)
	{
		t += "error in code";
		return t;
	}
	t += my.vout.at(0).scriptPubKey.hex;
	return t;
}

bool getrawtransactionlist(std::string & account, vector<my_rawtransactionlist> & my_transactions)
{
	Value ret;
	Array par;
	par.push_back(account);
	bool allok=true;
	try {
		ret = listtransactions(par, false);
	} catch (runtime_error ex) {
		allok=false;
		return allok;
	} catch (Object ex) {
		allok=false;
		return allok;
	}
	
	Value val;
	int size;
	my_rawtransactionlist my;
	try{
		if (ret.type() == array_type)
		{
			Array & params = ret.get_array();
			int size=params.size();
			for(int i = 0; i < size; i++)
			{
				//outs += ",hallo-1";
				my.clear();
				Object obj = params[i].get_obj();
				val = find_value(obj, "account");
				if(val.type() != null_type)
				{
					my.account = val.get_str();
					//outs += ",hallo-2";
				}
				val = find_value(obj, "address");
				if(val.type() != null_type)
				{
					my.address = val.get_str();
					//outs += ",hallo-3";
				}
				val = find_value(obj, "category");
				if(val.type() != null_type)
				{
					my.category = val.get_str();
					//outs += ",hallo-4";
				}
				val = find_value(obj, "amount");
				if(val.type() != null_type)
				{
					my.amount = val.get_real();
					//outs += ",hallo-5";
				}
				val = find_value(obj, "confirmations");
				if(val.type() != null_type)
				{
					//outs += ",hallo-27";
					my.confirmations = val.get_int();
				}
				val = find_value(obj, "generated");
				if(val.type() != null_type)
				{
					//outs += ",hallo-28";
					my.generated = val.get_bool();
				}
				val = find_value(obj, "blockhash");
				if(val.type() != null_type)
				{
					//outs += ",hallo-29";
					my.blockhash = val.get_str();
					//outs+=",";
					//outs+=get_string_of(my.time);
				}
				val = find_value(obj, "blockindex");
				if(val.type() != null_type)
				{
					//outs += ",hallo-29";
					my.blockindex = val.get_int();
					//outs+=",";
					//outs+=get_string_of(my.time);
				}
				val = find_value(obj, "blocktime");
				if(val.type() != null_type)
				{
					//outs += ",hallo-30";
					my.blocktime = val.get_int64();
					//outs+=",";
					//outs+=get_string_of(my.blocktime);
				}
				val = find_value(obj, "txid");
				if(val.type() != null_type)
				{
					//outs += ",hallo-30";
					my.txid = val.get_str();
					//outs+=",";
					//outs+=get_string_of(my.blocktime);
				}
				val = find_value(obj, "time");
				if(val.type() != null_type)
				{
					//outs += ",hallo-30";
					my.time = val.get_int64();
					//outs+=",";
					//outs+=get_string_of(my.blocktime);
				}
				val = find_value(obj, "timereceived");
				if(val.type() != null_type)
				{
					//outs += ",hallo-30";
					my.timereceived = val.get_int64();
					//outs+=",";
					//outs+=get_string_of(my.blocktime);
				}
				my.empty=false;
				my_transactions.push_back(my);
			}
		}
	} catch (runtime_error ex) {
		allok=false;
		return allok;
	} catch (Object ex) {
		allok=false;
		return allok;
	}
	return allok;
}

bool getrawtransactionlist_multisig(std::string & account, vector<my_rawtransactionlist> & my_transactions)
{
	vector<my_rawtransactionlist> my_transactions2;
	bool allok = getrawtransactionlist(account, my_transactions2);
	if(my_transactions2.size() == 0 || !allok)
	{
		return false;
	}
	int size = my_transactions2.size();
	for(int i = 0; i < size; i++)
	{
		if(!hasRedeemScript(my_transactions2.at(i).address))
			continue;
		my_transactions.push_back(my_transactions2.at(i));
	}
	allok=my_transactions.size()!=0;
	return allok;
}

Value listtransactions_multisig(const Array& params, bool fHelp)
{
	if (fHelp || params.size() != 1)
        throw runtime_error("fick die henne\n");
	vector<my_rawtransactionlist> my_transactions;
	string x = "";
	x+=params[0].get_str();
	bool allok = getrawtransactionlist_multisig(x, my_transactions);
	Array arr;
	if(my_transactions.size() == 0 || !allok)
	{
		return arr;
	}
	int size = my_transactions.size();
	for(int i = 0; i < size; i++)
	{
		Object entry;
		entry.push_back(Pair("account",my_transactions.at(i).account));
		entry.push_back(Pair("address",my_transactions.at(i).address));
		entry.push_back(Pair("category",my_transactions.at(i).category));
		entry.push_back(Pair("amount",my_transactions.at(i).amount));
		entry.push_back(Pair("confirmations",my_transactions.at(i).confirmations));
		entry.push_back(Pair("generated",my_transactions.at(i).generated));
		entry.push_back(Pair("blockhash",my_transactions.at(i).blockhash));
		entry.push_back(Pair("blockindex",my_transactions.at(i).blockindex));
		entry.push_back(Pair("blocktime",my_transactions.at(i).blocktime));
		entry.push_back(Pair("txid",my_transactions.at(i).txid));
		entry.push_back(Pair("time",my_transactions.at(i).time));
		entry.push_back(Pair("timereceived",my_transactions.at(i).timereceived));
		arr.push_back(entry);
	}
	return arr;
}

bool getrawlistunspent(vector<my_rawlistunspent> & my_unspenttransactions)
{
	Value ret;
	Array par;
	par.push_back(0);
	bool allok=true;
	try {
		ret = listunspent(par, false);
	} catch (runtime_error ex) {
		allok=false;
		return allok;
	} catch (Object ex) {
		allok=false;
		return allok;
	}
	
	Value val;
	int size;
	my_rawlistunspent my;
	try{
		if (ret.type() == array_type)
		{
			Array & params = ret.get_array();
			int size=params.size();
			for(int i = 0; i < size; i++)
			{
				//outs += ",hallo-1";
				my.clear();
				Object obj = params[i].get_obj();
				val = find_value(obj, "txid");
				if(val.type() != null_type)
				{
					my.txid = val.get_str();
					//outs += ",hallo-2";
				}
				val = find_value(obj, "vout");
				if(val.type() != null_type)
				{
					my.vout = val.get_int();
					//outs += ",hallo-3";
				}
				val = find_value(obj, "address");
				if(val.type() != null_type)
				{
					my.address = val.get_str();
					//outs += ",hallo-4";
				}
				val = find_value(obj, "scriptPubKey");
				if(val.type() != null_type)
				{
					my.scriptPubKey = val.get_str();
					//outs += ",hallo-5";
				}
				val = find_value(obj, "amount");
				if(val.type() != null_type)
				{
					//outs += ",hallo-27";
					my.amount = val.get_real();
				}
				val = find_value(obj, "confirmations");
				if(val.type() != null_type)
				{
					//outs += ",hallo-28";
					my.confirmations = val.get_int();
				}
				val = find_value(obj, "redeemScript");
				if(val.type() != null_type)
				{
					//outs += ",hallo-29";
					my.redeemScript = val.get_str();
					//outs+=",";
					//outs+=get_string_of(my.time);
				}
				my.empty=false;
				my_unspenttransactions.push_back(my);
			}
		}
	} catch (runtime_error ex) {
		allok=false;
		return allok;
	} catch (Object ex) {
		allok=false;
		return allok;
	}
	return allok;
}

bool getrawlistunspent_multisig(vector<my_rawlistunspent> & my_unspenttransactions)
{
	bool allok=false;
	vector<my_rawlistunspent> my_unspenttransactions2;
	allok=getrawlistunspent(my_unspenttransactions2);
	if(!allok)
		return allok;
	int size=my_unspenttransactions2.size();
	if(size==0)
		return false;
	allok=false;
	for(int i = 0; i < size;i++)
	{
		if(!my_unspenttransactions2.at(i).hasRedeemScript())
		{
			continue;
		}
		my_unspenttransactions.push_back(my_unspenttransactions2.at(i));
		allok=true;
	}
	return allok;
}
bool getrawlistunspentbyinformation_multisig(string & address_or_account, vector<my_rawlistunspent> & my_unspenttransactions)
{
	if(!hasRedeemScript(address_or_account))
	{
		my_multisigaddress my;
		if(!GetMultisigAccountAddress(address_or_account, my))
			return false;
		address_or_account=my.address;
	}
	vector<my_rawlistunspent> my_unspenttransactions2;
	bool allok=getrawlistunspent(my_unspenttransactions2);
	if(!allok)
		return allok;
	int size=my_unspenttransactions2.size();
	if(size==0)
		return false;
	allok=false;
	for(int i = 0; i < size;i++)
	{
		if(my_unspenttransactions2.at(i).address.compare(address_or_account)!=0)
		{
			continue;
		}
		my_unspenttransactions.push_back(my_unspenttransactions2.at(i));
		allok=true;
	}
	return allok;
}

Value listunspent_multisig(const Array& params, bool fHelp)
{
	if (fHelp || params.size() > 1)
        throw runtime_error("fick die henne\n");
	bool set=params.size()!=1;
	vector<my_rawlistunspent> my_unspenttransactions;
	bool allok;
	string x;
	if(!set)
		x+=params[0].get_str();
	if(set)
		allok = getrawlistunspent_multisig(my_unspenttransactions);
	else
		allok = getrawlistunspentbyinformation_multisig(x, my_unspenttransactions);
	Array arr;
	if(my_unspenttransactions.size() == 0 || !allok)
	{
		return arr;
	}
	
	int size = my_unspenttransactions.size();
	for(int i = 0; i < size; i++)
	{
		Object entry;
		entry.push_back(Pair("txid",my_unspenttransactions.at(i).txid));
		entry.push_back(Pair("vout",my_unspenttransactions.at(i).vout));
		entry.push_back(Pair("address",my_unspenttransactions.at(i).address));
		entry.push_back(Pair("scriptPubKey",my_unspenttransactions.at(i).scriptPubKey));
		entry.push_back(Pair("redeemScript",my_unspenttransactions.at(i).redeemScript));
		entry.push_back(Pair("amount",my_unspenttransactions.at(i).amount));
		entry.push_back(Pair("confirmations",my_unspenttransactions.at(i).confirmations));
		arr.push_back(entry);
	}
	return arr;
}

/*string get_vouts(string reedemScript,vector<my_rawtransactionlist> & my_transactions)
{
	int size  = my_transactions.size();
	my_transactioninformation my;
	bool all_ok;
	string ret;
	ret+="[";
	bool useReedemScript=reedemScript.compare("")==0;
	for(int i = 0; i < size; i++)
	{
		all_ok = getrawtransactiondetails(my_transactions.at(i).txid, my);
		if(!all_ok)
			continue;
		int jSize = my.vout.size();
		for(int j = 0; j < jSize; j++)
		{
			ret+="{\"txid\":\"";
			ret+=my.txid;
			ret+="\"";
			ret+=",\"vout\":";
			ret+=my.vout.at(i).n;
			ret+="\"scriptPubKey\":\"";
			ret+=my.vout.at(i).scriptPubKey.hex;
			ret+="\"";
			if(useReedemScript)
			{
				ret+=",\"reedemScript\":\"";
				ret+=reedemScript;
				ret+="\"";
			}
			ret+="}";
			if(j+1!=jSize)
				ret+=",";
		}
		my.clear();
		if(i+1!=size)
			ret+=",";
	}
	ret+="]";
}
*/
bool mygetnewaddress(std::string strAccount, std::string & myaddress)
{
	if (!pwalletMain->IsLocked())
      pwalletMain->TopUpKeyPool();

    // Generate a new key that is added to wallet
    CPubKey newKey;
    if (!pwalletMain->GetKeyFromPool(newKey, false))
        return false;
    CKeyID keyID = newKey.GetID();

    pwalletMain->SetAddressBookName(keyID, strAccount);

    myaddress=CBitcoinAddress(keyID).ToString();
	return true;
}

bool buildtransaction_multisig(std::string & account_or_address, std::string & receive_address, double amount, double fee, Array & params)
{
	if(fee<=0)
		fee=0.00001;
	my_multisigaddress my;
	if(!hasRedeemScript(account_or_address))
	{
		if(!GetMultisigAccountAddress(account_or_address,my))
		{
				return false;
		}
		account_or_address=my.address;
	}
	if(amount <= 0 || fee <= 0)
		return false;
	if(amount<=fee)
		return false;
	string change_address;
	string change_account="multisig_change_address";
	bool allok = mygetnewaddress(change_account, change_address);
	if(!allok)
		return false;
	std::vector<my_rawlistunspent> my_unspenttransactions;
	allok = getrawlistunspentbyinformation_multisig(account_or_address, my_unspenttransactions);
	if(!allok)
	{
		return false;
	}
	Array arr;
	double tAmount=amount+fee;
	double currentAmount = 0;
	int size=my_unspenttransactions.size();
	for(int i = 0; i < size; i++)
	{
			if(currentAmount >= tAmount)
			{
				break;
			}
			Object obj;
			obj.push_back(Pair("txid", my_unspenttransactions.at(i).txid));
			obj.push_back(Pair("vout", my_unspenttransactions.at(i).vout));
			obj.push_back(Pair("scriptPubKey", my_unspenttransactions.at(i).scriptPubKey));
			obj.push_back(Pair("redeemScript", my_unspenttransactions.at(i).redeemScript));
			arr.push_back(obj);
			currentAmount+=my_unspenttransactions.at(i).amount;
			if(i+1==size)
				return false;
	}
	params.push_back(arr);
	Object obj2;
	double diff=currentAmount-amount+fee;
	obj2.push_back(Pair(receive_address,amount));
	if(diff>0)
		obj2.push_back(Pair(change_address,diff));
	params.push_back(obj2);
}

Value createtransaction_multisig(const Array& params, bool fHelp)
{
	if (fHelp || params.size() < 4 || params.size() > 4)
        throw runtime_error("fick die henne\n");
	string account_or_address=params[0].get_str();
	string receive_address=params[1].get_str();
	double amount = params[2].get_real();
	double fee = params[3].get_real();
	Array arr;
	bool allok = buildtransaction_multisig(account_or_address, receive_address, amount, fee, arr);
	if(!allok)
	{
		arr.clear();
	} 
	return arr;
}

Value createrawtransaction_multisig(const Array& params, bool fHelp)
{
	if (fHelp || params.size() < 4 || params.size() > 4)
        throw runtime_error("fick die henne\n");
	string account_or_address=params[0].get_str();
	string receive_address=params[1].get_str();
	double amount = params[2].get_real();
	double fee = params[3].get_real();
	Array arr;
	bool allok = buildtransaction_multisig(account_or_address, receive_address, amount, fee, arr);
	if(!allok)
	{
		arr.clear();
	} 
	Value ret;
	allok=true;
	try {
		ret = createrawtransaction(arr,false);
	} catch (runtime_error ex) {
		allok=false;
		return allok;
	} catch (Object ex) {
		allok=false;
		return allok;
	}
	allok=ret.type()==str_type;
	if(!allok)
	{
		return allok;
	}
	string x = ret.get_str();
	Object obj;
	obj.push_back(Pair("txhash", x));
	obj.push_back(Pair("signdata", arr[0]));
	obj.push_back(Pair("fromaddress", account_or_address));
	return obj;
}	

Value decoderawtransaction_multisig(const Array& params, bool fHelp)
{
	bool allok=false;
	if (fHelp || params.size() != 1)
			throw runtime_error("fick die henne\n");
	try
	{
		
	} catch (runtime_error ex) {
		allok=false;
		return allok;
	} catch (Object ex) {
		allok=false;
		return allok;
	} catch (std::exception ex) {
		allok=false;
		return allok;
	}
}

string encodeBase64Data(unsigned char * data, size_t & len)
{
	unsigned char c[len];
	memcpy((void*)&c[0],(void*)&data[0],len);
	return EncodeBase64(c,len);
}

void decodeBase64Data(string & data, vector<unsigned char> & cpy, size_t & size)
{
	bool fDefault=false;
	cpy = DecodeBase64(data.c_str(), &fDefault);
	size = (size_t)cpy.size();
}

void decodeEnding(vector<unsigned char> & cpy, unsigned char * data, size_t & len)
{
		for(int i = 0; i < len; i++)
		{
			data[i]=cpy.at(i);
		}
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
	string rawtransaction = "'[{\"txid\":\"MY_TXID\",\"vout\":VOUT,\"scriptPubKey\":\"SCRIPT_PUB_KEY\",\"redeemScript\":\"REEDEM_SCRIPT\"}]' '{\"RECEIVER\":100,\"your_change_address\":50}'";
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
