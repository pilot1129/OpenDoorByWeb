// 사전 준비작업
// npm 과 nodejs는 필수로 설치해야함
// 앞으로 모듈(require)들은 전부 cmd -> 디렉토리 이동 -> npm install로 설치
// npm init 으로 디렉토리 생성
// npm install 을 통해 require('example') 안에 example 설치 ( 예시 : npm install https)
// 실행은 node Server.js 실행.
var fs = require('fs'); // 보안
var https = require('https'); // 설치
var express = require('express') // 설치
var app = express();
var bodyParser = require('body-parser');
var path = require('path');
var cookieParser = require('cookie-parser'); // 설치(쿠키모듈)
var mysql = require('sync-mysql'); // 설치(동기식 / 비동기하면 난리남
var crypto = require('crypto'); // 설치(RSA모듈)

// 인증서 들어가는 부분(Web-Server)
// sslcert/server.key 부분에 ssl 인증서 경로가 들어가야 함
var privateKey  = fs.readFileSync('public/verification/Web/private.key', 'utf8');
var certificate = fs.readFileSync('public/verification/Web/private.crt', 'utf8');
var credentials = {key: privateKey, cert: certificate};

// 인증서 들어가는 부분(Server-PI)
var privateKeyPI  = fs.readFileSync('public/verification/PI/private.key', 'utf8'); // private-key.pem
var certificatePI = fs.readFileSync('public/verification/PI/private.crt', 'utf8'); // public-cert.pem
var credentialsPI = {key: privateKey, cert: certificate}; // {key: privateKeyPI, cert: certificatePI} 수정 요망★★★★★★★★★★★★★★★

// 데이터베이스 구축
var connection = new mysql({
  host : 'localhost', //DESKTOP-MMJH35V
  user : 'root',
  password : '0000',
  port : '3306',
  database : 'mydb'
})

// HTTPS 서버 구축
var httpsServer = https.createServer(credentials, app); //Web-Server
var httpsServerPI = https.createServer(credentialsPI, function(socket){ // Server-PI
  socket.write("Message"); // 메세지 전송
  socket.on('data', function(data){ // 수신한 데이터 프린트
    console.log('Recieved: %s [it is %d bytes long]', data.toString().replace(/(\n)/gm,""), data.length);
  });
  socket.on('end',function(){ // Transmission over
    console.log("EOT"); 
  });
});

// 암호비교 Default = 0, Login 성공 1, PW오류 2, ID오류 3, Error = 4
function personalFunc(id,password,flag){
  console.log('비밀번호 일치 분석');
  let IDresult = connection.query("SELECT ID FROM userdata WHERE ID='" + id + "'");
  if(IDresult[0] != undefined)//아이디가 존재하면
  {
    console.log(IDresult);
    let PWresult = connection.query("SELECT PW FROM userdata WHERE ID='" + id + "' AND PW = '" + password + "'");
    if(PWresult[0] != undefined){ // PW 일치
      console.log(PWresult);
      return flag = 1;
    }
    else{ // PW오류
      return flag = 2;
    }
  }
  else
    return flag = 3;
  return flag = 0;
};

//회원가입 아이디 비교
function AddFunc(id,password,serial,check){
  console.log('아이디 중복 검사');
  let NewID = connection.query("SELECT ID FROM userdata WHERE ID = '" + id + "'");
  if(NewID[0] != undefined) // 이미 존재
    return check = true;
  else // 없는경우
    {
      var IotKey = "0"; // temp
      console.log("Regist Clear");
      var sqlSent="INSERT INTO userdata (id,pw,iotnum,iotkey) VALUES (?,?,?,?)"
      var params = [id, password,serial,IotKey];
      connection.query(sqlSent,params); // 삽입
      return check = false;
    }
  return check = false;
}


/////////////////////////// ROUTING ////////////////

// 가장 기본이 되는 라우팅 url: https://localhost:4000 입력시 실행
// 첫번째 입력페이지는 이 JS파일과 동일 디렉토리에 있어야 함.
// GET 파라미터 (초기 진입은 GET으로 실행했었음)
app.use(bodyParser.urlencoded({ extended: false}));
app.use(express.static(path.join(__dirname, 'public')));
app.use(cookieParser('Nightmare')); // 쿠키 사용, Parser안에는 임의의 문자열 추가

//GetMain
app.get('/', function(request,response){ // '/'라는 경로로 접속하면 아래의 것을 띄워줌.
  if(request.cookies.Check){ // 이 홈페이지의 쿠키인지를 확인해서 전송
    console.log(request.cookies);
    fs.readFile('LockPage.html',function(error,data){
      if(error)
        console.log("error");
      else{
        response.writeHead(200,{'Content-Type' :'text/html'});
        response.end(data);
      }
    });
  } // 쿠키 확인(쿠키가 있다면)
  else
  {
    fs.readFile('LoginPage.html',function(error,data){
      if(error)
        console.log("error");
      else{
        response.writeHead(200,{'Content-Type' :'text/html'});
        response.end(data);
      }
    });
  } // 쿠키가 없다면
});

//Get SignUp
app.get('/signup', function(request,response){ // '/'라는 경로로 접속하면 아래의 것을 띄워줌.
  fs.readFile('SignUp.html',function(error,data){
    if(error)
      console.log("error");
    else{
      response.writeHead(200,{'Content-Type' :'text/html'});
      response.end(data);
      }
  });
});

// POST 파라미터(Login)
app.post('/login', function(request,response){
  // post 내 숨겨저 전달된 정보를 parsing, 즉 해부하여 사용하기 위한 과정
  var id = request.body.id;
  var password = request.body.password;
  var flag = 0; //Default = 0, Login 성공 1, PW오류 2, ID오류 3, Error = 4
  // 개인이 만든 함수
  flag = personalFunc(id,password,flag);
  console.log("플래그 변환후 플래그 " + flag);
  if( flag ==1 ){ // 쿠키를 두개 생성. 하나는 홈페이지 확인용, 하나는 로그인용
    response.cookie('id',request.body.id,{
      //signed:true, // 이거 넣으면 인증이 안됨... 왜일까?
      secure : true,
      maxAge : 1000*60*60, //1시간
      httpOnly : true,
      encode : String
    }); // 유저 확인용 쿠키 생성
    response.cookie('Check','Nightmare',{
      //signed:true, // 이거 넣으면 인증이 안됨... 왜일까?
      secure : true,
      maxAge : 1000*60*60, //1시간
      httpOnly : true,
      encode : String
    }); // 홈페이지 확인용 쿠키 생성
    response.sendFile(__dirname+'/LockPage.html'); // 디렉토리 설정 후 파일전송
  }
  else if(flag == 2){ // PW오류
    response.sendFile(__dirname+'/LoginPage.html'); // 디렉토리 설정 후 파일전송
    response.send("2FLAG");
    }
  else if(flag == 3){ // ID오류
    response.sendFile(__dirname+'/LoginPage.html'); // 디렉토리 설정 후 파일전송
  }
  else{ // ERROR
    response.sendFile(__dirname+'/LoginPage.html'); // 디렉토리 설정 후 파일전송
  }
});

//Post Param(회원가입)
app.post('/signup', function(request,response){
  // post 내 숨겨저 전달된 정보를 parsing, 즉 해부하여 사용하기 위한 과정
  var id = request.body.id;
  var password = request.body.password;
  var serial = request.body.serial;
  var check = false;
  check = AddFunc(id,password,serial,check);
  if(check){ //아이디 이미 존재하나 안하나 체크
    response.sendFile(__dirname+'/LoginPage.html'); 
  }
  else{ // ERROR
    response.sendFile(__dirname+'/LoginPage.html'); 
  }
});


//Server Listen
httpsServer.listen(4000,function(){
  console.log('HTTPS Server opened PORT : 4000');
});
httpsServerPI.listen(5000,function(){
  console.log('HTTPS Server opened PORT : 5000');
})

/* 현재오류
  *  1. 쿠키 있을때 메인페이지를 갈 수가 없음. => 사실 상관은 없음
  *  2. Signed로 감싸면 쿠키가 안먹힘... => 이건 왜일까
  *  3. SSL 인증서 두개 제대로 받아야함.
  *  4. 암호화 일방향(bcrypt)? 대칭키(crypto)? 공개키?
  *  5. TLS 테스트 안해봄
  *  6. 오류별 팝업 띄워야함.
  * https://riptutorial.com/ko/node-js/example/19326/tls-%EC%86%8C%EC%BC%93---%EC%84%9C%EB%B2%84-%EB%B0%8F-%ED%81%B4%EB%9D%BC%EC%9D%B4%EC%96%B8%ED%8A%B8 참고(TLS)
*/