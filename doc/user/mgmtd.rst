.. _mgmtd:

*****
MGMTD
*****

The role of management daemon is to act as a centralized entity which takes
requests from the clients (NetConf, RestConf, Grpc, VTYSH) and passes it on
to the daemons (StaticD, BGPD) and returns the response to the clients. It
also acts as a central repository of YANG trees so that the computationally
heavy tasks can be offloaded from the daemons. It utilizes the Northbound
infrastructure to process the requests and generate the YANG diffs between the
running DB and candidate DB which it passes to the daemons. It’s advantages
can be summarized as below:

1. Consolidation and management of Running DB by a single entity.
2. Better control over configuration validation, commit and rollback.
3. Faster collection of ‘show running-config’.
4. Remove burden of YANG data parsing and validations (model-wise) away from Backend daemons
5. Improve performance and avoid CPU hogging of Backend daemons while loading huge configuration or retrieving huge operational dataset.

The management infrastructure can be divided into seperate componenets:

.. _mgmt_frntnd:

MGMTD Frontend
=============
The frontend is divided into two parts, the client and server. The client are
libraries that can be loaded by the frontend clients like NetConf, RestConf
to make connection to the adapter. As per the request types of the frontend
clients, APIs are called which sends these requests to the adapter which in
turn passes the information further as part of the Transaction. When the
response is returned back to the client, based on the response type, call-backs
are called, which are defined by the individual front-end client. For example,
for SET-CONIFIG requests, a bunch of xpaths and values are sent, and success or
failure types are returned, whereas for GET-DATA requests, a single xpath is
send and all the xpaths and values are returned back. The messages between the
client and adapter are wrapped in protobuff messages. 

MGMTD Transaction
================
Transaction can be understood as the basic unit of processing in the new
infrastructure. To maintain this atomicity we take lock on the Candidate DB
when processing is going on, and unlock only at the time of Transaction clean
up. The transaction formation step starts at the frontend-adapter and the lock
is taken. As per the transaction type, the DBs are accessed. In case of
SET-CONFIG request type, as per the Northbound infrastructure, first the
configs are applied to the Candidate DB. Then the Running DB and Candidate DB
are compared for differences and schema based validation takes place. From the
diffs, the batches are created. Batches can be understood as the entity which
will be sent to the backend adapters which will in turn change the internal
states of daemons. The Batches consists of xpaths, value, and type of
operation to be performed. The interested adapters are found out by xpath
matching which is a static list of xpath regular expression mapped to backend
clients. Once the batches are sent to the adapters, an APPLY message is sent,
which modifies the internal state of the daemon by calling the appropriate
call-backs. Once the APPLY message is sent, the transaction is cleaned up.
For the GET-DATA request type, the xpaths and values from the DB are returned
to the frontend-adapter and the transaction is cleaned up.

MGMTD Backend
============
Like the frontend, the backend is also divided into client and adapters.
The individual daemons use the client library to make connection to the
adapters. The adapters sends the batches to the daemons by wrapping in
Protobuff messages. These messages modifies the daemon’s internal state
by calling the appropriate call-backs.

CONFIG commands
===============

.. clicmd:: mgmt set-config xpath WORD value WORD

    This command sends a set-config requests for the xpath with specific value.
    This command is used for testing purpose, but the same flow will be used
    by other front-end agents like NetConf.

.. clicmd:: mgmt delete-config xpath WORD

    This command sends a set-config request to delete the node at the given
    xpath unless it is a key-leaf node in which case it is not deleted.

.. clicmd:: mgmt load-config file WORD <merge|replace>

    This command loads configuration in JSON format from the filepath specified,
    and merges or replaces the Candidate DB as per the option specified.

.. clicmd:: mgmt save-config db-name WORD file WORD

    This command dumps the DB specified in the db-name into the file in JSON
    format. This command in not supported for the Operational DB.

.. clicmd:: mgmt commit-abort

    This command will abort any configuration present on the Candidate but not
    been applied to the Running DB.

.. clicmd:: mgmt commit-apply

    This command commits any uncommited changes in the Candidate DB to the
    Running DB. It also dumps a copy of the tree in JSON format into
    frr_startup.json.

.. clicmd:: mgmt commit-check

    This command validates the configuration but does not applies to the
    Running DB.


SHOW commands
=============

.. clicmd:: show mgmt backend-adapter all

    This command shows the backend adapter information and the clients/daemons
    connected to the adapters.

.. clicmd:: show mgmt backend-yang-xpath-registry

    This command shows which adapters are registers for the xpaths.

.. clicmd:: show mgmt frontend-adapter all [detail]

    This command shows the frontend adapter information and the clients
    connected to the adapters.

.. clicmd:: show mgmt transaction all

    Shows the list of transaction and bunch of information about the transaction.

.. clicmd:: show mgmt get-config [db-name WORD] xpath WORD

    This command returns the xpaths and values of the nodes of the subtree
    pointed by the xpath.
    
.. clicmd:: show mgmt database-contents db-name WORD [xpath WORD] [file WORD] format WORD

    This command dumps the subtree pointed by the xpath in JSON or XML format. If filepath is
    not present then the tree will be printed on the shell.

.. clicmd:: show mgmt yang-xpath-subscription WORD

Types of Front-End Messages
===========================
* INIT_SESSION_REQ<Client-Connection-Id>
* INIT_SESSION_REPLY<Client-Connection-Id, Session-Id>
* LOCK_DB_REQ <Session-Id, Database-Id>
* LOCK_DB_REPLY <Session-Id, Database-Id>
* UNLOCK_DB_REQ <Session-Id, Database-Id>
* UNLOCK_DB_REPLY <Session-Id, Database-Id>
* GET_CONFIG_REQ <Session-Id, Database-Id, Base-Yang-Xpath>
* GET_CONFIG_REPLY <Session-Id, Database-Id, Base-Yang-Xpath, Yang-Data-Set>
* SET_CONFIG_REQ <Session-Id, Database-Id, Base-Yang-Xpath, Delete>
* SET_CONFIG_REPLY <Session-Id, Database-id, Base-Yang-Xpath, Status>
* VALIDATE_CONFIG_REQ <Session-Id, Database-Id>
* VALIDATE_CONFIG_REPLY <Session-Id, Database-id>
* COMMIT_CONFIG_REQ <Session-Id, Source-Db-Id, Dest-Db-Id>
* COMMIT_CONFIG_REPLY <Session-Id, Source-Db-id, Dest-Db-Id, Status>
* GET_DATA_REQ <Session-Id, Database-Id, Base-Yang-Xpath>
* GET_DATA_REPLY <Session-Id, Database-id, Base-Yang-Xpath, Yang-Data-Set>
* REGISTER_NOTIFY_REQ <Session-Id, Database-Id, Base-Yang-Xpath>
* DATA_NOTIFY_REQ <Database-Id, Base-Yang-Xpath, Yang-Data-Set>
* CLOSE_SESSION_REQ<Session-Id>

Types of Back-End Messages
==========================
* SUBSCRIBE_REQ <Req-Id, Base-Yang-Xpath, Filter-Type>
* SUBSCRIBE_REPLY <Req-Id, Status>
* TRXN_REQ <Trxn-Id, Create>
* TRXN_REPLY <Trxn-Id, Status>
* CREATE_CFGDATA_REQ <Trxn-Id, Req-Id, Batch-Id, ConfigDataContents>
* CREATE_CFGDATA_ERROR <Trxn-Id, Req-Id, Batch-Id, Status>
* VALIDATE_CFGDATA_REQ <Trxn-Id, Batch-Id>
* VALIDATE_CFGDATA_REPLY <Trxn-Id, Batch-Id, Status, ErrorInfo>
* APPLY_CFGDATA_REQ <Trxn-Id, Batch-Id>
* APPLY_CFGDATA_REPLY <Trxn-Id, Batch-Id, Status, ErrorInfo>
* GET_OPERDATA_REQ <Trxn-Id, Base-Yang-Xpath, Filter-Type>
* GET_OPERDATA_REPLY <Trxn-Id, OperDataContents>
* ACTION_REQ <Trxn-Id, Yang-Xpath>
* ACTION_REPLY <Trxn-Id, Status>
