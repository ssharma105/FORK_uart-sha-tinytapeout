////////////////////////////////////////////////////////////////////////////////
// Project: Bumblebee ASIC v2? 
//
// Module: SHA
//
// Description: The SHA module drives the SHA3-512 hashing core (keccak.v) .  
//       
//      
// Authors:  William Brenneke <wdbrenne@iu.edu>
//
////////////////////////////////////////////////////////////////////////////////


`timescale 1ns/1ps

module sha
(
	//Interface
	input clk_i,			
	input rst_i,			
	input start, 			
	input [31:0] data_in, 	
	input data_last,
	input data_valid,
	output logic data_in_ready, 
	output logic [511:0] hash,	
	output reg out_valid		
);

	//Vars
	integer subchunk, nextsubchunk; 
	

// ----------------------------------------------
// SHA3_CORE: Hashes chunk of data
// ----------------------------------------------

    // SHA3-512 (keccak.v)
    logic [31 : 0] sha_in = 0;          // in
    logic sha_in_ready;             // in
    logic sha_is_last;              // in
    logic [1 : 0] sha_byte_num;     // in
    logic sha_buffer_full;          // out
    logic [511 : 0] sha_out;        // out
    logic sha_out_ready;            // out
    logic sha_reset;

    keccak SHA_CORE(
        // Inputs
        .clk(clk_i),
        .reset(sha_reset),
        .in(sha_in),
        .in_ready(sha_in_ready),
        .is_last(sha_is_last),
        .byte_num(sha_byte_num),
        // Outputs
        .buffer_full(sha_buffer_full),
        .out(sha_out),
        .out_ready(sha_out_ready)
    );

    
    reg [31:0] sha_in_n = 0;
    reg sha_is_last_n   = 0;
    reg sha_in_ready_n  = 0;
    reg out_valid_n     = 0;
    reg [1:0] sha_byte_num_n = 0;
    reg [511:0] hash_n = 0;
    
	enum { 
		SHA_START,
		SHA_FEED,
		SHA_WAIT, 
		SHA_COMPUTE,
		SHA_DONE
	} 
		state, nextstate; 
	
	always_ff @(posedge  clk_i) begin
        if (rst_i) begin
			sha_reset <= 1;
			state <= SHA_START;
	
        end else begin
			sha_reset <= 0;
            state <= nextstate;
			subchunk <= nextsubchunk; 
			sha_in <= sha_in_n;
			sha_is_last <= sha_is_last_n;
			sha_in_ready <= sha_in_ready_n;
			sha_byte_num <= sha_byte_num_n;
			out_valid <= out_valid_n;
			hash <= hash_n;
		end
    end

	always_comb begin
		nextstate = state;
		data_in_ready = ~sha_buffer_full;
		
		sha_in_n = sha_in;
		sha_is_last_n = sha_is_last;
		sha_in_ready_n = sha_in_ready;
		sha_byte_num_n = sha_byte_num;
		out_valid_n = out_valid;
		hash_n = hash;
			
		case(state)
			SHA_START: begin
				sha_in_ready_n = 0;
				sha_is_last_n = 0;
				sha_byte_num_n = 0;
				nextstate = SHA_FEED;
				nextsubchunk = 0;
				out_valid_n = 0;
			end

			SHA_FEED: begin
			     sha_in_ready_n = data_valid;
				if(sha_buffer_full != 1) begin
					if(data_valid) begin
						sha_in_n = data_in;
						sha_byte_num_n = 0;
						nextstate = SHA_FEED;
					end
					if(data_last) begin
						sha_in_n = 0;
    	                sha_byte_num_n = 0;
        	            sha_is_last_n = 1;
            	        nextstate = SHA_COMPUTE;
					end
				end
				else 
					nextstate = SHA_FEED;
			end
			SHA_COMPUTE: begin
				sha_is_last_n = 0;
				sha_in_ready_n = 0;
				if(sha_out_ready) begin
					nextstate = SHA_DONE;
			     end
			end	
			SHA_DONE: begin
				hash_n = sha_out;
				out_valid_n = 1;
			end
		endcase
	end
endmodule
