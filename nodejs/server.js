/*server.js*/


var express = require('express');
var path = require('path');

// const hostname = '127.0.0.1';
// const port = 3000;


// var router = require('express').Router();

app = express();
// app.use(function (req, res, next) {
// 	console.log("New connection. " + req.connection.remoteAddress + "\r\n");
// 	next();
// });

// app.use('/', router);




app.set('port', process.env.PORT || 3000);
var http = require('http');

http.createServer(app).listen(app.get('port'), function () {
	console.log('Express server listening on port ' + app.get('port'));
});


app.get('/', function (req, res) {
	var ip = req.header('x-forwarded-for') || req.connection.remoteAddress;
	console.log('New connection..' + ip);
	
	res.statusCode = 200;
	res.setHeader('Content-Type', 'text/plain');
	res.end('Hello World\n');
});