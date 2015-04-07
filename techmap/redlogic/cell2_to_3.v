module NAND2(A, B, Y);
input A, B;
output Y;

NAND3 n(.A(A), .B(A), .C(B), .Y(Y));

endmodule


module AND2(A, B, Y);
input A, B;
output Y;

AND3 n(.A(A), .B(A), .C(B), .Y(Y));

endmodule
