<html>

<body>

<div id="sotatbl">

<?php
/*************************************************************************
 * PHP CODE STARTS HERE
 */

function print_sota_table() {
	session_start();
	$username=$_SESSION['username'];
	$password=$_SESSION['password'];
	$database=$_SESSION['database'];
	$tablehdr=$_SESSION['tablehdr'];

	mysql_connect(localhost,$username,$password);
	@mysql_select_db($database) or die( "Unable to select database");


	$query="select * from sotadb.sotatbl ORDER BY id DESC";
	$sotatbl=mysql_query($query);

	$veh_rows=mysql_numrows($sotatbl);
	$veh_cols=mysql_num_fields($sotatbl);


	/* print vehicles - HEADER */
	echo "<table border=1 width=100%><tr>";
	$i=0;while ($i < $veh_cols) {
		$meta = mysql_fetch_field($sotatbl, $i);
		$head = $tablehdr[$meta->name];
		echo "<th bgcolor=#ffdfcf height=30>$head</th>";
		$i++;
	}
	echo "</tr>";

	/* print vehicles - DATA */
	$i=0;while ($i < $veh_rows) {
		$row=mysql_fetch_row($sotatbl);
		echo "<tr>";
		if($i & 1)
			$bgc = "#ffffff";
		else
			$bgc = "#eeeeef";

		$j=0;while ($j < $veh_cols) {
			if($j == 1) {
				echo "<td bgcolor=$bgc>" . "<font face=tahoma size=3>" . '<a href="ecu_table.php?content='. $row[$j] . '">' . $row[$j] . '</a>' . "</font>" . "</td>";
			}
			else
				echo "<td bgcolor=$bgc ><font face=tahoma size=3>$row[$j]</font></td>";

			$j++;
		}
		echo "</tr>";
		$i++;
	}
	echo "</table>";

	mysql_free_result($sotatbl);
	mysql_close();
}

print_sota_table();
echo "<font face=arial size=2 color=#206080>";
echo date('l jS \of F Y h:i:s A');
echo "</font>";
?>

</div>
</body>
</html>
