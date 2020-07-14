// /*server.js*/


// var express = require('express');
// var path = require('path');

// // var favicon = require('serve-favicon');
// var logger = require('morgan');
// var cookieParser = require('cookie-parser');
// var bodyParser = require('body-parser');
// // const hostname = '127.0.0.1';
// // const port = 3000;


// // var router = require('express').Router();

// // app = express();
// // app.use(function (req, res, next) {
// // 	console.log("New connection. " + req.connection.remoteAddress + "\r\n");
// // 	next();
// // });

// // app.use('/', router);

// // app.set('port', process.env.PORT || 8080);
// // var http = require('http');

// // http.createServer(app).listen(app.get('port'), function () {
// // 	console.log('Express server listening on port ' + app.get('port'));
// // });


// // app.get('/', function (req, res) {
// // 	var ip = req.header('x-forwarded-for') || req.connection.remoteAddress;
// // 	console.log('New connection..' + ip);
	
// // 	res.statusCode = 200;
// // 	res.setHeader('Content-Type', 'text/plain');
// // 	res.end('Hello World\n');
// // });



// const fs = require('fs');
// const session = require('express-session');
// var routes = require('./routes/index');
// var router = require('express').Router();
// app = express();


// // Require static assets from public folder
// app.use(express.static(path.join(__dirname, 'public')));

// // Set 'views' directory for any views 
// // being rendered res.render()
// app.set('views', path.join(__dirname, 'views'));

// // Set view engine as EJS
// app.engine('html', require('ejs').renderFile);
// app.set('view engine', 'html');


// // view engine setup
// // app.set('views', path.join(__dirname, 'views'));


// // app.set('view engine', 'ejs');
// // var expressLayouts = require('express-ejs-layouts');
// // app.use(expressLayouts);
// app.set("layout extractScripts", true);
// app.set('layout extractStyles', true);

// // uncomment after placing your favicon in /public
// // app.use(favicon(__dirname + '/public/favicon.ico'));
// app.use(logger('dev'));
// app.use(bodyParser.json());
// app.use(bodyParser.urlencoded({ extended: true }));
// app.use(cookieParser());
// app.use(require('stylus').middleware(path.join(__dirname, 'public')));
// app.use(express.static(path.join(__dirname, 'public')));


// app.use(session({
//     secret: 'secrettexthere',
//     proxy: true,
//     resave: true,
//     saveUninitialized: true
// }));
// app.use(function (req, res, next) {
//     res.locals.user = req.session.user;
//     next();
// });

// app.use('/', routes);
// // app.use(app.router);
// // routes.initialize(app);


// // catch 404 and forward to error handler
// app.use(function (req, res, next) {
//     var err = new Error('Not Found');
//     err.status = 404;
//     next(err);
// });

// if (app.get('env') === 'development') {
//     app.use(function (err, req, res, next) {
//         res.status(err.status || 500);
//         res.render('error', {
//             message: err.message,
//             error: err
//         });
//     });
// }

// // production error handler
// // no stacktraces leaked to user
// app.use(function (err, req, res, next) {
//     res.status(err.status || 500);
//     res.render('error', {
//         message: err.message,
//         error: {}
//     });
// });

// ////***************************************************************/////
// app.set('port', process.env.PORT || 8080);
// var http = require('http');

// http.createServer(app).listen(app.get('port'), function () {
//     console.log('Express server listening on port ' + app.get('port'));
// });

// /////*****************
// app.set('trust proxy', true);
// app.use(function (req, res, next) {
//     if (req.headers['x-arr-ssl'] && !req.headers['x-forwarded-proto']) {
//         req.headers['x-forwarded-proto'] = 'https';
//     }
//     return next();
// });


/*
 * --------------------------------------------------------------------------------------------
 */
// const NodeMediaServer = require('node-media-server');

// const config = {
//   rtmp: {
//     port: 1935,
//     // chunk_size: 60000,
//     gop_cache: true,
//     ping: 30,
//     ping_timeout: 60
//   },
//   http: {
//     port: 8000,
//     allow_origin: '*'
//   }
// };

// var nms = new NodeMediaServer(config)
// nms.run();

/*
*-----------------------------------------------------------------------------------------------
 */

const RtmpServer = require('rtmp-server');
const rtmpServer = new RtmpServer();

rtmpServer.on('error', err => {
  throw err;
});

rtmpServer.on('client', client => {
  //client.on('command', command => {
  //  console.log(command.cmd, command);
  //});

  client.on('connect', () => {
     console.log('connect', client.app);
  });
  
  client.on('play', ({ streamName }) => {
    console.log('PLAY', streamName);
  });
  
  client.on('publish', ({ streamName }) => {
    console.log('PUBLISH', streamName);
  });
  
  client.on('stop', () => {
    console.log('client disconnected');
  });
});

rtmpServer.listen(1935);