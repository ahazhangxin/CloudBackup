#include<fstream>
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include"httplib.h"
#include<boost/filesystem.hpp>
#include<iostream>
#include<unistd.h>
#include<fcntl.h>
#include"compress.hpp"

#define SERVER_BASE_DIR "www"
#define SERVER_ADDR "0.0.0.0"
#define SERVER_PORT 9000
#define SERVER_BACKUP_DIR SERVER_BASE_DIR"/list/"

using namespace httplib;
namespace bf = boost::filesystem;

CompressStore cstor;

class CloudServer
{
  private:
    SSLServer srv;
  public:
    CloudServer(const char *cacert, const char *privkey):srv(cacert,privkey)
    {
      bf::path base_path(SERVER_BASE_DIR);
      if(!bf::exists(base_path)){
        bf::create_directory(base_path);
      }
      bf::path list_path(SERVER_BACKUP_DIR);
      if(!bf::exists(list_path)){
        bf::create_directory(list_path);
      }
    }


    bool Start(){
      //srv.set_base_dir(SERVER_BASE_DIR);
      srv.Get("/(list(/){0,1}){0,1}",GetFileList);
      //正则表达式(./)通配任意字符
      srv.Get("/list/(.*)", GetFileData);
      srv.Put("/list/(.*)", PutFileData);
      srv.listen(SERVER_ADDR,SERVER_PORT);
      return true;
    }
  private:

    //接受上传并拆分其中的http头信息
    static void PutFileData(const Request &req,Response &rsp){
      //使用的是http1.1,支持Range，防止因传输问题而存储了错误的文件
      //如果没有Range则返回400
      if(!req.has_header("Range")){
        rsp.status = 400;
        return;
      }
      std::string range = req.get_header_value("Range");
      //获取文件的范围，用RangeParse读取,并将字符串转化为整形
      int64_t range_start;
      if(RangeParse(range , range_start) == false){
        rsp.status = 400;
        return;
      }
      std::cout << "backup file:[" << req.path <<"] range:["<< range <<"]\n";
      std::string realpath = SERVER_BASE_DIR + req.path;
      cstor.SetFileData(realpath, req.body, range_start);
      return;
    }

    static bool RangeParse(std::string &range, int64_t &start)
    {
      //bytes=start-end
      size_t pos1 = range.find("=");
      size_t pos2 = range.find("-");
      if(pos1 == std::string::npos || pos2 == std::string::npos){
        std::cerr << "range:[" << range << "] format error\n";
        return false;
      }
      std::stringstream rs;
      rs << range.substr(pos1 + 1, pos2 - pos1 - 1);
      rs >> start;
      return true;
    }
    

    //迭代文件目录
    //文件列表请求功能A调用在compress.hpp中Getfilelist实现对
  static void GetFileList(const Request &req,Response &rsp)
  {
      (void)req;
      std::vector<std::string> list;

      cstor.GetFileList(list);
      std::string body;
      body = "<html><body><ol><hr />";
      //用迭代器浏览目录
      for(auto i : list){
        //如果是目录，则continue
        //如果是文件，则传递信息
        bf::path path(i);
        std::string file = path.filename().string();
        std::string uri = "/list/" + file;
        body += "<h4><li>";
        body += "<a href='";
        body += uri;
        body += "'>";
        body += file;
        body += "</a>";
        body += "</li></h4>";
        //<html><body><ol><hr /><h4><li><a href=' uri '></a><hr /></ol></body></html>
      }
      body += "<hr /></ol></body></html>";

      rsp.set_content(&body[0],"text/html");
      //content-type
      //因为是要显示出来的页面，所以，
      return;
    }


    //下载文件
    static void GetFileData(const Request &req,Response &rsp){
      std::string realpath = SERVER_BASE_DIR + req.path;
      std::string body;

      cstor.GetFileData(realpath, body);
      rsp.set_content(body, "text/plain");
    }

};
void thr_start()
{
  cstor.LowHeatFileStore();
}

int main()
{
  //daemon(1,1);
  std::thread thr(thr_start);
  thr.detach();
  CloudServer srv("./cacert.pem", "./privkey.pem");
  srv.Start();
  return 0;
}
