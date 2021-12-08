const WebSocket = require('ws');
const https = require('https');
const fs = require('fs');
const logger = console.log;

const MAX_USERS = 50;
let userIdList = new Array(MAX_USERS);
for (let i = 0; i < MAX_USERS; i++) {
   userIdList[i] = (Math.random() * 100000) >> 0; 
}

const cfg = {
    port: 8010,
    ssl_key: './auth/server.key',
    ssl_cert: './auth/server.crt'
};

const processRequest = (req, res) => {
    res.writeHead(200);
    res.end('厉害了，我的WebSockets!\n');
};

const app = https.createServer({
    key: fs.readFileSync(cfg.ssl_key),
    cert: fs.readFileSync(cfg.ssl_cert)
}, processRequest).listen(cfg.port);

const wsServer = new WebSocket.Server({
    server: app
});

// client to userInfo map
let clientMap = {};
const util = {
    getUserList (clientMap) {
        let userList = [];
        for (let key in clientMap) {
            let user = clientMap[key];
            userList.push({
                roomid: user.roomid,
                userId: user.id,
                ip: user.ip,
                loginTime: user.loginTime
            });
        }
        return userList;
    },
    broadcast (data) {
        wsServer.clients.forEach(client => {
            if (client.readyState === WebSocket.OPEN) {
                client.sendData(data);
            }
        });
    }
};
let msgHandler = {
    // 注册
    regist (data, client) {
        let user = clientMap[client._userId];
        user.roomid = data.roomid;
        logger(`client ${client._ip} join room ${data.roomid}`);
        client.sendData({
            type: 'userInfo',
            data: {
                roomid: user.roomid,
                id: client._userId,
                ip: client._ip
            }
        });

        util.broadcast({
            type: 'userList',
            data: util.getUserList(clientMap)
        });
    },
    // 注销
    unregist (data, client) {
        let user = clientMap[client._userId];
        user.roomid = '';
        logger(`client ${client._ip} leave room ${data.roomid}`);
        client.sendData({
            type: 'userInfo',
            data: {
                roomid: user.roomid,
                id: user.id,
                ip: user.ip
            }
        });

        util.broadcast({
            type: 'userList',
            data: util.getUserList(clientMap)
        });
        client.roomid = '';
    },
    // 呼叫
    setUpCall (data, client) {
        let remoteUser = clientMap[data.remoteUserId];
        if (!remoteUser) {
            let msg = `client id ${data.remoteUserId} not found`;
            console.error(msg);
            client.sendError(msg);
            return;
        } 
        client._remote = remoteUser.client;
        remoteUser.client._remote = client;
        remoteUser.client.sendData({
            type: 'recvCall',
            data: {
                userId: client._userId,
                ip: client._ip
            }
        });
    },
    // 拒绝电话
    rejectCall (data, client) {
        let remote = client._remote;
        remote.sendData({
            type: 'callRejected'
        });
    },
    // 同意聊天
    answerCall (data, client) {
        let remote = client._remote;
        remote.sendData({
            type: 'callAnswered'
        });
    },
    candidate (data, client) {
        let remote = client._remote;
        remote.sendData({
            type: 'remoteCandidate',
            data: {
                candidate: data.candidate
            }
        });
    },
    offer (data, client) {
        let remote = client._remote;
        remote.sendData({
            type: 'offer',
            data: {
                desc: data.desc
            }
        });
    },
    answer (data, client) {
        let remote = client._remote;
        remote.sendData({
            type: 'answer',
            data: {
                desc: data.desc
            }
        });
    }
};
wsServer.on('connection', (client, request) => {
    client.sendData = data => {
        client.send(JSON.stringify(data));
    };
    client.sendError = msg => {
        client.sendData({
            type: 'error',
            data: {
                msg
            }
        });
    };
    if (!userIdList.length) {
        client.sendData({
            type: 'error',
            msg: 'User Id exhausted! Please wait'
        });
        client.terminate();
        return;
    }
    let remoteIp = request.connection.remoteAddress.replace('::ffff:', '');
    logger(`client ${remoteIp} connected`);
    let userId = userIdList.pop();
    // 发送当前用户信息
    // client.send(JSON.stringify({
    //     type: 'userInfo',
    //     data: {
    //         id: userId,
    //         ip: remoteIp
    //     }
    // }));
    // 发送当前在线的所有用户信息
    client._userId = userId;
    client._ip = remoteIp;
    client.on('message', msg => {
        console.log(`recv data from ${client._ip}: `);
        console.log(msg);
        let data = null;
        try {
            data = JSON.parse(msg);
        } catch (e) {
            client.sendError('Unresolved data format');
            console.error(e);
            return;
        }
        if (typeof msgHandler[data.type] === 'function') {
            try {
                msgHandler[data.type](data.data, client);
            } catch (e) {
                console.error(e);
            }
        } else {
            client.sendError('Unresolved data type');
            return;
        }
    });
    client.on('close', () => {
        delete clientMap[userId];
        userIdList.push(client._userId);
        logger(`client ${client._ip} closed`);
        clearTimeout(client._closetId);
        util.broadcast({
            type: 'userList',
            data: util.getUserList(clientMap)
        });
    });
    clientMap[userId] = {
        id: userId,
        ip: remoteIp,
        loginTime: Date.now(),
        roomid: "",
        client: client
    };

    // util.broadcast({
    //     type: 'userList',
    //     data: util.getUserList(clientMap)
    // });
    // 24 hours 后自动关闭
    client._closetId = setTimeout(() => {
        client.terminate();
    }, 1000*60*60*24); 
});
