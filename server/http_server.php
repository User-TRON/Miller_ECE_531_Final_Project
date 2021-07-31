<?php

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
function process_get($sql_connection){
//  echo "process_get\n";

  if( $_GET["id"] ) {
      $id = htmlspecialchars($_GET["id"]);
//    echo "id = $id \n";
//    echo "result = ";
    $result = mysqli_query($sql_connection, "SELECT * FROM http_server WHERE id = $id");

    while($query_data = mysqli_fetch_row($result)) {
      echo json_encode(unserialize($query_data[1]));
    }
    echo "\n";

    exit();
  }

}//process_get

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
function process_post($sql_connection){
//  echo "process_post\n";    
  $data = json_decode(file_get_contents('php://input'),true);
//  print_r($data);

//  echo "serialize json data\n";
  $serial_data = serialize($data);
//  print_r($serial_data);

  //save to mysql
  $result = mysqli_query($sql_connection, "INSERT INTO http_server (json) VALUES ('$serial_data')");

  //return id

  $id = mysqli_insert_id($sql_connection);
//  echo "New id =";
  echo json_encode($id);
  echo "\n";
  exit();
}//process_post

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
function process_put($sql_connection){
//  echo "process_put\n";    

  if( $_GET["id"] ) {
      $id = htmlspecialchars($_GET["id"]);
//    echo "id = $id \n";
      
  $data = json_decode(file_get_contents('php://input'),true);
//  print_r($data);

//  echo "serialize json data\n";
  $serial_data = serialize($data);
//  print_r($serial_data);

  //save to mysql
  $result = mysqli_query($sql_connection, "REPLACE INTO http_server (id,json) VALUES ($id,'$serial_data')");
    exit();
  }

  exit();
}//process_put

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
function process_delete($sql_connection){
//  echo "process_delete\n";    

  if( $_GET["id"] ) {
    $id = htmlspecialchars($_GET["id"]);
//    echo "id = $id \n";
//    echo "result = ";
    $result = mysqli_query($sql_connection, "DELETE FROM http_server WHERE id = $id");

    exit();
  }

}//process_delete


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
function handle_error(){
  echo "Invalid request\n";
  exit();
}

  //Setup Database and Connect

  include "../inc/dbinfo.inc"; 
  $sql_connection = new mysqli(DB_SERVER, DB_USERNAME, DB_PASSWORD);

  if (mysqli_connect_errno()) echo "Failed to connect to MySQL: " . mysqli_connect_error();
//  echo "Connection Successful to MYSQL Database";

  $server_database = mysqli_select_db($sql_connection, DB_DATABASE);

  $query = "CREATE TABLE IF NOT EXISTS http_server (
      id INT NOT NULL PRIMARY KEY AUTO_INCREMENT, 
      json VARCHAR(1000) NOT NULL)";

  if(!mysqli_query($sql_connection, $query)) echo("<p>Error creating table.</p>");

//  echo "Table is good";


  $method = $_SERVER['REQUEST_METHOD'];
//  echo "Method = $method \n";

  switch ($method) {
    case 'PUT':
      process_put($sql_connection);  
      break;

    case 'POST':
      process_post($sql_connection);  
      break;

    case 'GET':
      process_get($sql_connection);  
      break;

    case 'DELETE':
      process_delete($sql_connection);
      break;

    default:
      handle_error();  
      break;
  }


  mysqli_free_result($result);
  mysqli_close($sql_connection);


?>


<!DOCTYPE html>
<html>
<body>
<h1>ECE 531 HTTP Server</h1>

</body>
</html>
